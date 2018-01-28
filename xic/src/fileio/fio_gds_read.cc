
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
#include "fio_chd.h"
#include "fio_layermap.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "miscutil/texttf.h"
#include "miscutil/timedbg.h"


// When converting to files, write the header properties to this file.
#define HDR_PRP_FILE "gds_header_props"


// Return true if the file passes a simple GDSII identity test.
//
bool
cFIO::IsGDSII(FILE *fp)
{
    FilePtr file = sFilePtr::newFilePtr(fp);
    bool ret = gds_in::check_file(file, 0, 0);
    delete file;
    return (ret);
}


// Read the GDSII file gds_fname, the new cells will be be added to
// the database.  All conversions will be scaled by the value of
// scale.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cFIO::DbFromGDSII(const char *gds_fname, const FIOreadPrms *prms,
    stringlist **tlp, stringlist **tle)
{
    if (!prms)
        return (OIerror);
    Tdbg()->start_timing("gds_read");
    gds_in *gds = new gds_in(prms->allow_layer_mapping());
    gds->set_show_progress(true);
    gds->set_no_test_empties(IsNoCheckEmpties());

    CD()->SetReading(true);
    gds->assign_alias(NewReadingAlias(prms->alias_mask()));
    gds->read_alias(gds_fname);
    bool ret = gds->setup_source(gds_fname);
    if (ret) {
        gds->log_version();
        gds->set_to_database();
    }
    if (ret) {
        Tdbg()->start_timing("gds_read_phys");
        gds->begin_log(Physical);
        CD()->SetDeferInst(true);
        ret = gds->parse(Physical, false, prms->scale());
        CD()->SetDeferInst(false);
        Tdbg()->stop_timing("gds_read_phys");
        if (ret)
            ret = gds->mark_references(tlp);
        gds->check_ptype4();
        gds->end_log();
    }
    if (ret && !CD()->IsNoElectrical() && gds->has_electrical()) {
        Tdbg()->start_timing("gds_read_elec");
        gds->begin_log(Electrical);
        CD()->SetDeferInst(true);
        bool lpc = CD()->EnableLabelPatchCache(true);
        ret = gds->parse(Electrical, false, prms->scale());
        CD()->EnableLabelPatchCache(lpc);
        CD()->SetDeferInst(false);
        Tdbg()->stop_timing("gds_read_elec");
        if (ret)
            ret = gds->mark_references(tle);
        gds->end_log();
    }
    if (ret)
        gds->mark_top(tlp, tle);
    CD()->SetReading(false);


    if (ret && !gds->write_gds_header_props())
        ifPrintCvLog(IFLOG_WARN, "could not write header properties file.");
    if (ret)
        gds->dump_alias(gds_fname);

    OItype oiret = ret ? OIok : gds->was_interrupted() ? OIaborted : OIerror;
    delete gds;
    Tdbg()->stop_timing("gds_read");
    return (oiret);
}


// Read the GDSII file gds_fname, performing translation.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cFIO::FromGDSII(const char *gds_fname, const FIOcvtPrms *prms,
    const char *chdcell)
{
    if (!prms) {
        Errs()->add_error("FromGDSII: null destination pointer.");
        return (OIerror);
    }
    if (!prms->destination()) {
        Errs()->add_error("FromGDSII: no destination given!");
        return (OIerror);
    }

    cCHD *chd = CDchd()->chdRecall(gds_fname, false);
    if (chd) {
        if (chd->filetype() != Fgds) {
            Errs()->add_error("FromGDSII:: CHD file type not GDSII!");
            return (OIerror);
        }

        // We were given a CHD name, use it.
        gds_fname = chd->filename();
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
                tab->read_alias(gds_fname);
            cvINFO info = cvINFOtotals;
            if (prms->ecf_level() == ECFall || prms->ecf_level() == ECFpre)
                info = cvINFOplpc;
            chd = NewCHD(gds_fname, Fgds, Physical, tab, info);
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

    gds_in *gds = new gds_in(prms->allow_layer_mapping());
    gds->set_show_progress(true);

    // Translating, directly streaming.  Skip electrical data if
    // StripForExport is set.

    const cv_alias_info *aif = prms->alias_info();
    if (aif)
        gds->assign_alias(new FIOaliasTab(true, false, aif));
    else {
        unsigned int mask = prms->alias_mask();
        if (prms->filetype() == Fgds)
            mask |= CVAL_GDS;
        gds->assign_alias(NewTranslatingAlias(mask));
    }
    gds->read_alias(gds_fname);

    bool ret = gds->setup_source(gds_fname);
    if (ret) {
        gds->log_version();
        ret = gds->setup_destination(prms->destination(), prms->filetype(),
            prms->to_cgd());
    }

    if (ret) {
        gds->begin_log(Physical);
        ret = gds->parse(Physical, false, prms->scale());
        gds->check_ptype4();
        gds->end_log();
    }
    if (ret && !fioStripForExport && !prms->to_cgd() &&
            !CD()->IsNoElectrical() && gds->has_electrical()) {
        gds->begin_log(Electrical);
        ret = gds->parse(Electrical, false, prms->scale());
        gds->end_log();
    }

    if (ret && !gds->write_gds_header_props())
        ifPrintCvLog(IFLOG_WARN,
            "could not write header properties file.");
    if (ret)
        gds->dump_alias(gds_fname);

    OItype oiret = ret ? OIok : gds->was_interrupted() ? OIaborted : OIerror;
    delete gds;
    SetAllowPrptyStrip(false);
    return (oiret);
}
// End of cFIO functions


namespace {
    const char *gds_record_names[] =
    {
        "HEADER",        // 0
        "BGNLIB",        // 1
        "LIBNAME",       // 2
        "UNITS",         // 3
        "ENDLIB",        // 4
        "BGNSTR",        // 5
        "STRNAME",       // 6
        "ENDSTR",        // 7
        "BOUNDARY",      // 8
        "PATH",          // 9
        "SREF",          // 10
        "AREF",          // 11
        "TEXT",          // 12
        "LAYER",         // 13
        "DATATYPE",      // 14
        "WIDTH",         // 15
        "XY",            // 16
        "ENDEL",         // 17
        "SNAME",         // 18
        "COLROW",        // 19
        "TEXTNODE",      // 20
        "SNAPNODE",      // 21
        "TEXTTYPE",      // 22
        "PRESENTATION",  // 23
        "SPACING",       // 24
        "STRING",        // 25
        "STRANS",        // 26
        "MAG",           // 27
        "ANGLE",         // 28
        "UINTEGER",      // 29
        "USTRING",       // 30
        "REFLIBS",       // 31
        "FONTS",         // 32
        "PATHTYPE",      // 33
        "GENERATIONS",   // 34
        "ATTRTABLE",     // 35
        "STYPTABLE",     // 36
        "STRTYPE",       // 37
        "ELFLAGS",       // 38
        "ELKEY",         // 39
        "LINKTYPE",      // 40
        "LINKKEYS",      // 41
        "NODETYPE",      // 42
        "PROPATTR",      // 43
        "PROPVALUE",     // 44

        // R5 options
        "BOX",           // 45
        "BOXTYPE",       // 46
        "PLEX",          // 47
        "BGNEXTN",       // 48
        "ENDEXTN",       // 49
        "TAPENUM",       // 50
        "TAPECODE",      // 51
        "STRCLASS",      // 52
        "reserved",      // 53
        "FORMAT",        // 54
        "MASK",          // 55
        "ENDMASKS",      // 56
        "LIBDIRSIZE",    // 57
        "SRFNAME",       // 58
        "LIBSECUR",      // 59
        "BORDER",        // 60
        "SOFTFENCE",     // 61
        "HARDFENCE",     // 62
        "SOFTWIRE",      // 63
        "HARDWIRE",      // 64
        "PATHPORT",      // 65
        "NODEPORT",      // 66
        "USERCONSTRAINT",// 67
        "SPACER_ERROR",  // 68
        "CONTACT"        // 69
    };
}


void
gds_info::add_record(int rtype)
{
    rec_counts[rtype]++;
    cv_info::add_record(rtype);
}


char *
gds_info::pr_records(FILE *fp)
{
    char buf[256];
    sLstr lstr;
    for (int i = 0; i < GDS_NUM_REC_TYPES; i++) {
        if (rec_counts[i]) {
            sprintf(buf, "%-16s %d\n", gds_record_names[i], rec_counts[i]);
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
        }
    }
    return (lstr.string_trim());
}
// End of gds_info functions


gds_in::gds_in(bool allow_layer_mapping) : cv_in(allow_layer_mapping)
{
    in_filetype = Fgds;

    in_magn = 1.0;
    in_angle = 0.0;
    in_munit = 0.0;
    in_obj_offset = 0;
    in_attr_offset = 0;
    in_offset_next = 0;
    in_fp = 0;
    in_recsize = 0;
    in_rectype = II_EOF;
    in_elemrec = 0;
    in_curlayer = -1;
    in_curdtype = -1;
    in_layer = 0;
    in_dtype = 0;
    in_nx = 1, in_ny = 1;
    in_attrib = 0;
    in_presentation = 0;
    in_numpts = 0;
    in_pwidth = 0;
    in_ptype = 0;
    in_text_width = 0;
    in_text_height = 0;
    in_plexnum = 0;
    in_bextn = 0;
    in_eextn = 0;
    in_version = 0;
    in_text_flags = 0;
    in_ptype4_cnt = 0;
    in_sprops = 0;
    in_eprops = 0;

    in_layer_name[0] = 0;
    in_layers = 0;
    in_undef_layers = 0;
    in_phys_layer_tab = 0;
    in_elec_layer_tab = 0;
    in_headrec_ok = false;
    in_bswap = false;
    in_reflection = false;
    in_string = 0;
    *in_cellname = 0;
    *in_cbuf = 0;

    ftab[II_HEADER]         = &gds_in::a_header;
    ftab[II_BGNLIB]         = &gds_in::a_bgnlib;
    ftab[II_LIBNAME]        = &gds_in::a_libname;
    ftab[II_UNITS]          = &gds_in::a_units;
    ftab[II_ENDLIB]         = &gds_in::nop;
    ftab[II_BGNSTR]         = &gds_in::a_bgnstr;
    ftab[II_STRNAME]        = &gds_in::a_strname;
    ftab[II_ENDSTR]         = &gds_in::nop;
    ftab[II_BOUNDARY]       = &gds_in::a_boundary;
    ftab[II_PATH]           = &gds_in::a_path;
    ftab[II_SREF]           = &gds_in::a_sref;
    ftab[II_AREF]           = &gds_in::a_aref;
    ftab[II_TEXT]           = &gds_in::a_text;
    ftab[II_LAYER]          = &gds_in::a_layer;
    ftab[II_DATATYPE]       = &gds_in::a_datatype;
    ftab[II_WIDTH]          = &gds_in::a_width;
    ftab[II_XY]             = &gds_in::a_xy;
    ftab[II_ENDEL]          = &gds_in::nop;
    ftab[II_SNAME]          = &gds_in::a_sname;
    ftab[II_COLROW]         = &gds_in::a_colrow;
    ftab[II_TEXTNODE]       = &gds_in::nop;
    ftab[II_SNAPNODE]       = &gds_in::a_snapnode;
    ftab[II_TEXTTYPE]       = &gds_in::a_datatype;
    ftab[II_PRESENTATION]   = &gds_in::a_presentation;
    ftab[II_SPACING]        = &gds_in::nop;
    ftab[II_STRING]         = &gds_in::a_string;
    ftab[II_STRANS]         = &gds_in::a_strans;
    ftab[II_MAG]            = &gds_in::a_mag;
    ftab[II_ANGLE]          = &gds_in::a_angle;
    ftab[II_UINTEGER]       = &gds_in::nop;
    ftab[II_USTRING]        = &gds_in::nop;
    ftab[II_REFLIBS]        = &gds_in::a_reflibs;
    ftab[II_FONTS]          = &gds_in::a_fonts;
    ftab[II_PATHTYPE]       = &gds_in::a_pathtype;
    ftab[II_GENERATIONS]    = &gds_in::a_generations;
    ftab[II_ATTRTABLE]      = &gds_in::a_attrtable;
    ftab[II_STYPTABLE]      = &gds_in::nop;
    ftab[II_STRTYPE]        = &gds_in::nop;
    ftab[II_ELFLAGS]        = &gds_in::nop;
    ftab[II_ELKEY]          = &gds_in::nop;
    ftab[II_LINKTYPE]       = &gds_in::nop;
    ftab[II_LINKKEYS]       = &gds_in::nop;
    ftab[II_NODETYPE]       = &gds_in::a_datatype;
    ftab[II_PROPATTR]       = &gds_in::a_propattr;
    ftab[II_PROPVALUE]      = &gds_in::a_propvalue;
    // R5 options
    ftab[II_BOX]            = &gds_in::a_box;
    ftab[II_BOXTYPE]        = &gds_in::a_datatype;
    ftab[II_PLEX]           = &gds_in::a_plex;
    ftab[II_BGNEXTN]        = &gds_in::a_bgnextn;
    ftab[II_ENDEXTN]        = &gds_in::a_endextn;
    ftab[II_TAPENUM]        = &gds_in::unsup;
    ftab[II_TAPECODE]       = &gds_in::unsup;
    ftab[II_STRCLASS]       = &gds_in::unsup;
    ftab[53]                = &gds_in::unsup;
    ftab[II_FORMAT]         = &gds_in::unsup;
    ftab[II_MASK]           = &gds_in::unsup;
    ftab[II_ENDMASKS]       = &gds_in::unsup;
    ftab[II_LIBDIRSIZE]     = &gds_in::unsup;
    ftab[II_SRFNAME]        = &gds_in::unsup;
    ftab[II_LIBSECUR]       = &gds_in::unsup;
    ftab[II_BORDER]         = &gds_in::unsup;
    ftab[II_SOFTFENCE]      = &gds_in::unsup;
    ftab[II_HARDFENCE]      = &gds_in::unsup;
    ftab[II_SOFTWIRE]       = &gds_in::unsup;
    ftab[II_HARDWIRE]       = &gds_in::unsup;
    ftab[II_PATHPORT]       = &gds_in::unsup;
    ftab[II_NODEPORT]       = &gds_in::unsup;
    ftab[II_USERCONSTRAINT] = &gds_in::unsup;
    ftab[II_SPACER_ERROR]   = &gds_in::unsup;
    ftab[II_CONTACT]        = &gds_in::unsup;
}


gds_in::~gds_in()
{
    delete in_fp;
    for (Attribute *hcpy = in_sprops; hcpy; hcpy = in_sprops) {
        in_sprops = (Attribute*)hcpy->next_prp();
        delete hcpy;
    }
    delete in_undef_layers;
    delete [] in_string;

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
}


// Static function.
// Return true if the file looks like valid GDSII.  If the second arg is
// non-nil, the version number is returned, and the global things are set
// on failure.  If the third arg is non-nil, return whether bytes need
// to be swapped.
//
bool
gds_in::check_file(FilePtr fp, int *vers, bool *bswap)
{
    if (!fp)
        return (false);

    // byte swap and integrity test
    char cbuf[10];
    cbuf[0] = fp->z_getc();
    cbuf[1] = fp->z_getc();
    cbuf[2] = fp->z_getc();
    cbuf[3] = fp->z_getc();
    cbuf[4] = fp->z_getc();
    cbuf[5] = fp->z_getc();
    fp->z_rewind();
    int version;
    if (!is_gds(cbuf, &version))
        return (false);
    if (vers)
        *vers = version;
    if (bswap)
        *bswap = cbuf[0];
    return (true);
}


// Static function.
// Returns true if buf contains the first 4 bytes of a GDSII file.
// Also return the version number.
//
bool
gds_in::is_gds(char *buf, int *version)
{
    // GDSII Header:
    //  short bytecount (6) (big-endian)
    //  header code byte (0)
    //  data type byte  (2)
    //  short version (3) (big-endian)

    if (buf[0] == 0 && buf[1] == 6 && buf[2] == 0 && buf[3] == 2) {
        *version = (buf[4] << 8) + buf[5];
        return (true);
    }
    // byte swap ok
    if (buf[0] == 6 && buf[1] == 0 && buf[2] == 2 && buf[3] == 0) {
        *version = (buf[5] << 8) + buf[4];
        return (true);
    }
    *version = -1;
    return (false);
}


inline bool
gds_in::gds_read(void *p, unsigned int sz)
{
    if (in_fp->z_read(p, 1, sz) != (int)sz) {
        if (in_fp->z_eof())
            in_rectype = II_EOF;
        Errs()->add_error("Read error on input.");
        fatal_error();
        return (false);
    }
    if (in_bswap) {
        sz >>= 1;
        unsigned char *c = (unsigned char*)p;
        while (sz--) {
            unsigned char ct = *c++;
            *(c-1) = *c;
            *c++ = ct;
        }
    }
    return (true);
}


inline bool
gds_in::gds_seek(int64_t os)
{
    in_headrec_ok = false;
    if (in_fp->z_seek(os, SEEK_SET) < 0) {
        Errs()->add_error("z_seek failed.");
        return (false);
    }
    return (true);
}


inline int64_t
gds_in::gds_tell()
{
    return (in_fp->z_tell());
}


//
// Setup functions
//

// Open the source file.
//
bool
gds_in::setup_source(const char *gds_fname, const cCHD *chd)
{
    in_fp = sFilePtr::newFilePtr(gds_fname, "r");
    if (!in_fp) {
        Errs()->sys_error("open");
        Errs()->add_error("Can't open stream file %s.", gds_fname);
        return (false);
    }
    if (in_fp->file)
        in_gzipped = true;
    bool bswap;
    int version;
    if (!check_file(in_fp, &version, &bswap)) {
        Errs()->add_error(
            "File does not appear to contain Stream data.");
        return (false);
    }

    // The chd argument allows passing of the checksum for gzipped files
    // enabling use of random access table.
    if (chd && chd->crc())
        in_fp->z_set_crc(chd->crc());

    in_filename = lstring::copy(gds_fname);
    in_bswap = bswap;
    in_version = version;

    return (true);
}


// Set up the destination channel.
//
bool
gds_in::setup_destination(const char *destination, FileType ftype,
    bool to_cgd)
{
    if (destination) {
        in_out = FIO()->NewOutput(in_filename, destination, ftype, to_cgd);
        if (!in_out)
            return (false);
        in_own_in_out = true;
    }

    in_action = cvOpenModeTrans;

    ftab[II_BGNSTR]         = &gds_in::ac_bgnstr;
    ftab[II_BOUNDARY]       = &gds_in::ac_boundary;
    ftab[II_PATH]           = &gds_in::ac_path;
    ftab[II_SREF]           = &gds_in::ac_sref;
    ftab[II_AREF]           = &gds_in::ac_aref;
    ftab[II_TEXT]           = &gds_in::ac_text;
    ftab[II_PROPVALUE]      = &gds_in::ac_propvalue;
    ftab[II_BOX]            = &gds_in::ac_box;

    return (true);
}


// Explicitly set the back-end processor, and reset a few things.
//
bool
gds_in::setup_backend(cv_out *out)
{
    if (in_fp)
        in_fp->z_rewind();
    in_bytes_read = 0;
    in_fb_incr = UFB_INCR;
    in_offset = 0;
    in_out = out;
    in_own_in_out = false;

    in_action = cvOpenModeTrans;

    ftab[II_BGNSTR]         = &gds_in::ac_bgnstr;
    ftab[II_BOUNDARY]       = &gds_in::ac_boundary;
    ftab[II_PATH]           = &gds_in::ac_path;
    ftab[II_SREF]           = &gds_in::ac_sref;
    ftab[II_AREF]           = &gds_in::ac_aref;
    ftab[II_TEXT]           = &gds_in::ac_text;
    ftab[II_PROPVALUE]      = &gds_in::ac_propvalue;
    ftab[II_BOX]            = &gds_in::ac_box;

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
gds_in::parse(DisplayMode mode, bool listonly, double sc, bool save_bb,
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
            in_phys_info = new gds_info(pl, pc);
            in_phys_info->initialize();
        }
    }
    else {
        in_scale = 1.0;
        in_needs_mult = false;
        if (infoflags != cvINFOnone) {
            bool pl = (infoflags == cvINFOpl || infoflags == cvINFOplpc);
            bool pc = (infoflags == cvINFOpc || infoflags == cvINFOplpc);
            in_elec_info = new gds_info(pl, pc);
            in_elec_info->initialize();
        }
    }
    in_listonly = listonly;
#ifdef VIAS_IN_LISTONLY
    in_ignore_prop = listonly && (in_mode == Electrical);
#else
    in_ignore_prop = listonly;
#endif
    in_header_read = false;

    in_savebb = save_bb;
    in_ignore_text = in_mode == Physical && FIO()->IsNoReadLabels();

    bool ret = read_data();

    in_savebb = false;

    if (in_listonly) {

        // We keep a layer_tab during listonly for speed, but the
        // elements do not have CDl layers assigned.  Delete the
        // table, it will be remade when geometry is read.

        if (in_mode == Physical) {
            if (in_phys_layer_tab) {
                SymTabGen gen(in_phys_layer_tab, true);
                SymTabEnt *h;
                while ((h = gen.next()) != 0) {
                    delete (gds_lspec*)h->stData;
                    delete h;
                }
                delete in_phys_layer_tab;
                in_phys_layer_tab = 0;
            }
        }
        else {
            if (in_elec_layer_tab) {
                SymTabGen gen(in_elec_layer_tab, true);
                SymTabEnt *h;
                while ((h = gen.next()) != 0) {
                    delete (gds_lspec*)h->stData;
                    delete h;
                }
                delete in_elec_layer_tab;
                in_elec_layer_tab = 0;
            }
        }
    }
    return (ret);
}


//
// Entries for reading through CHD.
//

// Setup scaling and read the file header.
//
bool
gds_in::chd_read_header(double phys_scale)
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
gds_in::chd_read_cell(symref_t *p, bool use_inst_list, CDs **sdret)
{
    if (sdret)
        *sdret = 0;

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
        FIO()->ifPrintCvLog(IFLOG_WARN, "Unresolved cell %s (%s).",
            DisplayModeNameLC(in_mode), Tstring(p->get_name()),
            sd ? "in memory" : "not found");
        return (true);
    }

    if (!gds_seek(p->get_offset()))
        return (false);

    CD()->InitCoincCheck();
    in_uselist = true;
    in_obj_offset = 0;
    in_layer = in_dtype = in_attrib = -1;

    bool nbad;
    while ((nbad = get_record())) {
        if (in_rectype == II_BGNSTR)
            break;

        switch (in_rectype) {

        // extension for electrical properties
        case II_PROPATTR:
        case II_PROPVALUE:
            if (!(this->*ftab[in_rectype])())
                return (false);
            break;

        case II_EOF:
            Errs()->add_error("Premature end of file.");
            fatal_error();
            return (false);
        default:
            Errs()->add_error("Unexpected record type %d.", in_rectype);
            fatal_error();
            return (false);
        }
    }
    if (!nbad)
        return (false);

    cv_chd_state stbak;
    in_chd_state.push_state(p, use_inst_list ? get_sym_tab(in_mode) : 0,
        &stbak);
    bool ret = (this->*ftab[II_BGNSTR])();
    if (sdret)
        *sdret = in_sdesc;
    in_chd_state.pop_state(&stbak);

    if (!ret && in_out && in_out->was_interrupted())
        in_interrupted = true;

    return (ret);
}


cv_header_info *
gds_in::chd_get_header_info()
{
    if (!in_header_read)
        return (0);
    gds_header_info *gds = new gds_header_info(in_munit);
    in_header_read = false;
    return (gds);
}


void
gds_in::chd_set_header_info(cv_header_info *hinfo)
{
    if (!hinfo || in_header_read)
        return;
    gds_header_info *gdsh = static_cast<gds_header_info*>(hinfo);
    in_munit = gdsh->unit();
    in_header_read = true;
}


//
// Misc. entries.
//

// Return true if electrical records present.
//
bool
gds_in::has_electrical()
{
    bool found_eof = false;
    bool bad_data = false;

    if (in_action == cvOpenModePrint) {
        if (in_print_end > in_print_start && in_offset > in_print_end)
            return (false);
        if (in_print_reccnt == 0 || in_print_symcnt == 0)
            return (false);
    }

    // skip over padding
    int64_t posn = gds_tell();
    posn = GDS_PHYS_REC_SIZE - (posn % GDS_PHYS_REC_SIZE);
    if (posn != GDS_PHYS_REC_SIZE) {
        while (posn--) {
            if (in_fp->z_getc() == EOF) {
                // no electrical records
                in_fp->z_clearerr();
                found_eof = true;
                break;
            }
        }
    }

    if (!found_eof) {
        posn = gds_tell();

        // make sure that we are at a HEADER record
        unsigned byte0, byte1, rtype, dtype;
        if (!in_bswap) {
            byte0 = in_fp->z_getc();
            byte1 = in_fp->z_getc();
            rtype = in_fp->z_getc();
            dtype = in_fp->z_getc();
            in_fp->z_getc();
            in_fp->z_getc();
        }
        else {
            byte1 = in_fp->z_getc();
            byte0 = in_fp->z_getc();
            dtype = in_fp->z_getc();
            rtype = in_fp->z_getc();
            in_fp->z_getc();
            in_fp->z_getc();
        }
        if (!in_fp->z_eof()) {
            if (byte0*256 + byte1 == 6 && rtype == II_HEADER && dtype == 2) {
                // header looks ok
                if (!gds_seek(posn))
                    return (false);
                return (true);
            }
            bad_data = true;
        }
        in_fp->z_clearerr();
    }

    if (in_action == cvOpenModePrint) {
        fprintf(in_print_fp, ">> No ELECTRICAL records found\n");
        if (bad_data)
            fprintf(in_print_fp,
                ">> Warning: garbage found after ENDLIB record.\n");
    }
    else {
        FIO()->ifPrintCvLog(IFLOG_INFO, "No electrical records found.");
        if (bad_data)
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "File contains junk after physical data.");
    }
    return (false);
}


// Add the GDSII file header stuff as properties to the top-level
// physical cells.  This is called from mark_references().
//
void
gds_in::add_header_props(CDs *sd)
{
    if (in_sprops) {
        in_prpty_list = in_sprops;
        add_properties_db(sd, 0);
        in_prpty_list = 0;
    }
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
gds_in::has_geom(symref_t *p, const BBox *AOI)
{
    if (!p->get_defseen()) {
        // No cell definition in file, just ignore this.
        return (OIambiguous);
    }
    if (!gds_seek(p->get_offset()))
        return (OIerror);

    in_ignore_prop = true;
    in_uselist = true;

    bool nbad;
    while ((nbad = get_record())) {
        if (in_rectype == II_BGNSTR)
            break;
        switch (in_rectype) {
        case II_EOF:
            Errs()->add_error("Premature end of file.");
            return (OIerror);
        default:
            Errs()->add_error("Unexpected record type %d.", in_rectype);
            return (OIerror);
        }
    }
    if (!nbad)
        return (OIerror);

    in_cell_offset = in_offset;

    if (!get_record())
        return (OIerror);
    if (in_rectype != II_STRNAME) {
        Errs()->add_error("Unexpected record type %d.", in_rectype);
        return (OIerror);
    }
    if (!a_strname())
        return (OIerror);

    in_sdesc = 0;
    in_obj_offset = 0;
    in_attrib = -1;
    reset_layer_check();

    while ((nbad = get_record())) {
        if (in_rectype == II_ENDSTR)
            break;

        in_obj_offset = in_offset;
        if (in_rectype == II_EOF) {
            Errs()->add_error("Premature end of file.");
            return (OIerror);
        }
        if (in_rectype >= GDS_NUM_REC_TYPES) {
            unsup();
            continue;
        }
        switch (in_rectype) {
        case II_SREF:
        case II_AREF:
        case II_TEXT:
            if (!read_element())
                return (OIerror);
            delete [] in_string;
            in_string = 0;
            continue;

        case II_BOUNDARY:
        case II_PATH:
        case II_BOX:
            if (!read_element())
                return (OIerror);
            cl_type cvt = check_layer(0, false);
            if (cvt == cl_error)
                return (OIerror);
            if (cvt == cl_skip)
                continue;
            if (AOI) {
                Poly po(in_numpts, in_points);
                if (!po.intersect(AOI, false))
                    continue;
            }
            return (OIok);
        }
    }
    if (!nbad)
        return (OIerror);

    in_attr_offset = 0;
    in_cell_offset = 0;
    return (OIambiguous);
}
// End of virtual overrides


void
gds_in::log_version()
{
    if ((in_version >= 3 && in_version <= 7) ||
            (in_version >= 500 && in_version <= 800))
        FIO()->ifPrintCvLog(IFLOG_INFO, "Reading version %d library.",
            in_version);
    else
        FIO()->ifPrintCvLog(IFLOG_WARN, "nonstandard library version %d.",
            in_version);
}


bool
gds_in::write_gds_header_props()
{
    if (in_sprops) {
        if (in_action == cvOpenModeTrans) {
            if (!in_out->write_info(in_sprops, HDR_PRP_FILE))
                return (false);
        }
    }
    return (true);
}


void
gds_in::check_ptype4()
{
    if (in_ptype4_cnt > 0) {
        if (in_mode == Physical)
            FIO()->ifPrintCvLog(IFLOG_INFO,
            "Wires with pathtype 4 (%d found) were converted to pathtype 0.",
                in_ptype4_cnt);
        in_ptype4_cnt = 0;
    }
}


//
// Private Functions
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


// Read the file header, used before randomly accessing a cell.  The
// only thing that we really need out of there is the scale factor.
//
bool
gds_in::read_header(bool quick)
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

    bool nbad = true;
    bool begun = false;
    while ((nbad = get_record())) {
        if (in_rectype == II_ENDLIB)
            break;

        switch (in_rectype) {
        case II_BGNLIB:
            if (!quick && !(this->*ftab[in_rectype])())
                return (false);
            begun = true;
            break;
        case II_UNITS:
            if (!(this->*ftab[in_rectype])())
                return (false);
            break;
        case II_HEADER:
        case II_LIBNAME:
        case II_REFLIBS:
        case II_FONTS:
        case II_GENERATIONS:
        case II_ATTRTABLE:
            if (!quick && !(this->*ftab[in_rectype])())
                return (false);
            break;
        case II_EOF:
            if (begun) {
                Errs()->add_error("Premature end of file.");
                fatal_error();
                return (false);
            }
            return (false);
        default:
            if (in_mode == Physical)
                in_header_read = true;
            return (true);
        }
    }
    if (in_mode == Physical)
        in_header_read = true;
    return (nbad);
}


// This iterates through the Physical or Electrical records of the
// file.  The scale factor sc is always the physical scale factor,
// used in Electrical mode for fixing property strings.
//
bool
gds_in::read_data()
{
    in_ptype4_cnt = 0;
    in_obj_offset = 0;
    in_cell_offset = 0;
    in_attr_offset = 0;

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

    // read_header() has already read a record...
    bool nbad = true;
    do {
        if (in_rectype == II_ENDLIB) {
            if (in_action == cvOpenModeTrans && !in_cv_no_endlib) {
                if (!in_out->write_endlib(0))
                    return (false);
            }
            else if (in_action == cvOpenModePrint) {
                print_offset();
                print_space(4);
                if (in_mode == Physical)
                    print_word_end("LIBRARY (PHYSICAL)");
                else
                    print_word_end("LIBRARY (ELECTRICAL)");
            }
            break;
        }

        if (in_rectype == II_EOF) {
            if (in_action == cvOpenModePrint) {
                if (in_printing_done)
                    break;
                fprintf(in_print_fp, ">> Premature end of file.");
                return (false);
            }
            Errs()->add_error("Premature end of file.");
            fatal_error();
            return (false);
        }
        if (in_rectype >= GDS_NUM_REC_TYPES) {
            unsup();
            continue;
        }
        else if (!(this->*ftab[in_rectype])())
            return (false);
    } while ((nbad = get_record()));
    return (nbad);
}


bool
gds_in::read_element()
{
    in_layer = 0;
    in_pwidth = 0;
    in_dtype = 0;
    in_numpts = 0;
    in_presentation = 0;
    in_reflection = false;
    in_magn = 1;
    in_angle = 0;
    in_ptype = 0;
    in_plexnum = 0;
    in_bextn = 0;
    in_eextn = 0;
    in_attrib = 0;
    in_text_width = 0;
    in_text_height = 0;
    in_text_flags = 0;

    in_elemrec = in_rectype;
    bool nbad;
    while ((nbad = get_record())) {
        if (in_rectype == II_ENDEL) {
            in_elemrec = 0;
            if (in_action == cvOpenModePrint) {
                print_offset();
                print_space(8);
                print_word_end("ELEMENT");
            }
            break;
        }
        if (in_rectype == II_EOF) {
            if (in_action == cvOpenModePrint)
                return (true);
            Errs()->add_error("Premature end of file.");
            fatal_error();
            return (false);
        }
        if (in_rectype >= GDS_NUM_REC_TYPES) {
            unsup();
            continue;
        }
        if (!(this->*ftab[in_rectype])())
            return (false);
    }
    return (nbad);
}


// Read the Xic-specific text size property.
//
bool
gds_in::read_text_property()
{
    in_text_width = 0;
    in_text_height = 0;
    in_text_flags = 0;
    char *s = in_cbuf;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (!strcmp(tok, "width")) {
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok)
                break;
            in_text_width = atoi(tok);
            if (in_text_width < 0)
                in_text_width = 0;
        }
        else if (!strcmp(tok, "height")) {
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok)
                break;
            in_text_height = atoi(tok);
            if (in_text_height < 0)
                in_text_height = 0;
        }
        else if (!strcmp(tok, "show")) {
            in_text_flags |= TXTF_SHOW;
            in_text_flags &= ~TXTF_HIDE;
        }
        else if (!strcmp(tok, "hide")) {
            in_text_flags |= TXTF_HIDE;
            in_text_flags &= ~TXTF_SHOW;
        }
        else if (!strcmp(tok, "tlev"))
            in_text_flags |= TXTF_TLEV;
        else if (!strcmp(tok, "liml"))
            in_text_flags |= TXTF_LIML;
        delete [] tok;
    }
    return (true);
}


// Read the next record, return the record type in the argument.  Return
// false on fatal error or abort.
//
bool
gds_in::get_record()
{
    if (in_headrec_ok)
        in_offset = in_offset_next;
    else
        in_offset_next = in_offset = gds_tell();

    if (in_bytes_read > in_fb_incr) {
        show_feedback();
        if (in_interrupted) {
            Errs()->add_error("user interrupt");
            fatal_error();
            return (false);
        }
    }

    if (!in_headrec_ok) {
        if (!gds_read(in_headrec, 4)) {
            if (in_rectype == II_EOF)
                return (true);
            return (false);
        }
    }
    in_rectype = in_headrec[2];

    if (in_action == cvOpenModePrint) {
        in_printing = (in_offset >= in_print_start);
        if (in_print_reccnt > 0)
            in_print_reccnt--;
        if (in_print_reccnt == 0 || in_print_symcnt == 0 ||
                (in_print_end > in_print_start && in_offset > in_print_end)) {
            // Stop printing.
            in_printing_done = true;
            in_rectype = II_EOF;
            return (true);
        }
    }

    in_recsize = (in_headrec[0] << 8) | in_headrec[1];
    if (in_recsize & 1) {
        // "Stream records are always an even number of bytes"
        Errs()->add_error("Odd-size record found, size %d.", in_recsize);
        fatal_error();
        return (false);
    }

    in_headrec_ok = false;
    if (in_rectype == II_ENDLIB) {
        in_bytes_read += 4;
        in_offset_next += 4;
        return (true);
    }
    if (in_rectype == II_XY) {
        in_numpts = in_recsize >> 3;

        char *p;
        if (sizeof(int) == 4)
            p = (char*)in_points;
        else
            p = (char*)in_cbuf;

        if (!gds_read(p, in_recsize)) {
            if (in_rectype == II_EOF)
                return (true);
            return (false);
        }
        if (in_action == cvOpenModePrint) {
            for (int i = 0; i < in_numpts; i++) {
                in_points[i].x = long_value(&p);
                in_points[i].y = long_value(&p);
            }
        }
        else {
            for (int i = 0; i < in_numpts; i++) {
                in_points[i].x = scale(long_value(&p));
                in_points[i].y = scale(long_value(&p));
            }
        }
        memcpy(in_headrec, p, 4);
        in_headrec_ok = true;
    }
    else {
        if (!gds_read(in_cbuf, in_recsize)) {
            if (in_rectype == II_EOF)
                return (true);
            return (false);
        }
        memcpy(in_headrec, in_cbuf + in_recsize - 4, 4);
        in_headrec_ok = true;
        *(in_cbuf + in_recsize - 4) = 0;  // Null terminate strings
    }

    if (info())
        info()->add_record(in_rectype);

    in_bytes_read += in_recsize;
    in_offset_next += in_recsize;
    return (true);
}


void
gds_in::fatal_error()
{
    Errs()->add_error("Fatal error at offset %llu.",
        (unsigned long long)in_offset);
}


// Generate a warning log message, format is
//  string [cellname offset]
//
void
gds_in::warning(const char *str)
{
    FIO()->ifPrintCvLog(IFLOG_WARN, "%s [%s %llu]",
        str, in_cellname, (unsigned long long)
        (in_obj_offset ? in_obj_offset : in_offset));
}


// Generate a warning log message, format is
//  string [cellname x,y layername offset]
//
void
gds_in::warning(const char *str, int x, int y)
{
    char tbf[64];
    if (in_layer < 256 && in_dtype < 256)
        sprintf(tbf, "%02X%02X", in_layer, in_dtype);
    else
        sprintf(tbf, "%04X%04X", in_layer, in_dtype);

    FIO()->ifPrintCvLog(IFLOG_WARN, "%s [%s %d,%d %s %llu]", str,
        in_cellname, x, y, tbf, 
        (unsigned long long)(in_obj_offset ? in_obj_offset : in_offset));
}


// Generate a warning log message, format is
//  string for instance of mstr [cellname x,y offset]
//
void
gds_in::warning(const char *str, const char *mstr, int x, int y)
{
    FIO()->ifPrintCvLog(IFLOG_WARN,
        "%s for instance of %s [%s %d,%d %llu]",
        str, mstr, in_cellname, x, y,
        (unsigned long long)(in_obj_offset ? in_obj_offset : in_offset));
}


void
gds_in::date_value(tm *tm, char **p)
{
    memset(tm, 0, sizeof(struct tm));
    tm->tm_year = short_value(p);
    tm->tm_mon = short_value(p) - 1;
    tm->tm_mday = short_value(p);
    tm->tm_hour = short_value(p);
    tm->tm_min = short_value(p);
    tm->tm_sec = short_value(p);
}


//      Function to convert from STREAM to VAX double precision.
//      The argument is a integer buffer containing the eight bytes
//      of the STREAM double precision field.  The first character in
//      the buffer contains the exponent, the second contains the most
//      significant byte of the mantissa, etc.
//
//
//    VAX's double precision field:
//
//    Mantissa is base 2 (1/2 <= mantissa < 1).  Exponent is excess-128.
//
//              111111 1111222222222233 3333333344444444 4455555555556666
//    0123456789012345 6789012345678901 2345678901234567 8901234567890123
//    ---------------- ---------------- ---------------- ----------------
//    FFFFFFFEEEEEEEES FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
//    L     M          L              M L              M L              M
//
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
// conversions required, and portability is an issue.
//
double
gds_in::doubleval(char *s)
{
    unsigned char *b = (unsigned char*)s;
    int sign = 0;
    int exp = s[0];
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


// The action functions

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


// Look at the present layer/datatype mapping to internal layers, The
// list of internal layers to output is returned in in_layers.  If this
// is changed from previous, *changed is set true.  If no mapping
// exists, a new name is defined in in_layer_name.  If create_new, the
// new layer is created and added to the database.  Otherwise,
// in_layers is set to 0, and the caller must handle this case.
//
cl_type
gds_in::check_layer(bool *changed, bool create_new)
{
    const char *msg = "Mapping all datatypes for gds layer %d to layer %s.";
    const char *msg1 = "Mapping gds layer %d datatype %d to layer %s.";

    if (changed)
        *changed = false;

    /* Can't happen since layer is stored as an unsigned short.
    if (in_layer < 0 || in_layer >= GDS_MAX_LAYERS) {
        Errs()->add_error("check_layer: bad layer number %d.",
            in_layer);
        fatal_error();
        return (cl_error);
    }
    if (in_dtype < 0 || in_dtype >= GDS_MAX_DTYPES) {
        Errs()->add_error("check_layer: bad datatype number %d.",
            in_dtype);
        fatal_error();
        return (cl_error);
    }
    */

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

    unsigned long ll = (in_layer << 16) | in_dtype;

    gds_lspec *lspec = (gds_lspec*)SymTab::get(layer_tab, ll);
    if (lspec != (gds_lspec*)ST_NIL) {
        in_layer = lspec->layer;
        in_dtype = lspec->dtype;
        strncpy(in_layer_name, lspec->hexname, 12);
        in_layers = lspec->list;

        if (in_layer != in_curlayer || in_dtype != in_curdtype) {
            in_curlayer = in_layer;
            in_curdtype = in_dtype;
            if (changed)
                *changed = true;
            if (in_listonly && info())
                info()->add_layer(in_layer_name);
        }
        else if (changed && in_layers && in_layers->next)
            // writing multiple layers
            *changed = true;

        if (!in_layers && !in_layer_name[0])
            return (cl_skip);
        return (cl_ok);
    }

    if (in_layer >= GDS_MAX_SPEC_LAYERS || in_dtype >= GDS_MAX_SPEC_DTYPES) {
        FIO()->ifPrintCvLog(IFLOG_WARN,
            "overrange layer number %d or datatype %d encountered.",
            in_layer, in_dtype);
    }

    if (in_mode == Physical) {
        // See if layer is in skip list, before aliasing.
        if (in_lcheck && !in_lcheck->layer_ok(0, in_layer, in_dtype)) {
            layer_tab->add(ll, new gds_lspec(in_layer, in_dtype, 0, 0), false);
            return (cl_skip);
        }

        if (in_layer_alias) {
            char buf[16];
            strmdata::hexpr(buf, in_layer, in_dtype);
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
                    in_layer = lyr;
                    in_dtype = dt;
                }
            }
        }
    }

    if (in_listonly) {
        strmdata::hexpr(in_layer_name, in_layer, in_dtype);
        if (info())
            info()->add_layer(in_layer_name);
        layer_tab->add(ll, new gds_lspec(in_layer, in_dtype,
            in_layer_name, 0), false);
        in_curlayer = in_layer;
        in_curdtype = in_dtype;
        return (cl_ok);
    }

    in_layers = FIO()->GetGdsInputLayers(in_layer, in_dtype, in_mode);
    if (in_layers) {
        in_curlayer = in_layer;
        in_curdtype = in_dtype;
        if (changed)
            *changed = true;
        layer_tab->add(ll, new gds_lspec(in_layer, in_dtype, 0, in_layers),
            false);
        return (cl_ok);
    }

    bool error;
    CDl *ld = FIO()->MapGdsLayer(in_layer, in_dtype, in_mode,
        in_layer_name, create_new, &error);
    if (ld) {
        in_layers = new CDll(ld, 0);
        in_curlayer = in_layer;
        in_curdtype = in_dtype;
        if (FIO()->IsNoMapDatatypes())
            FIO()->ifPrintCvLog(IFLOG_INFO, msg, in_layer, ld->name());
        else
            FIO()->ifPrintCvLog(IFLOG_INFO, msg1, in_layer, in_dtype,
                ld->name());
        if (changed)
            *changed = true;
        layer_tab->add(ll, new gds_lspec(in_layer, in_dtype, 0, in_layers),
            false);
        return (cl_ok);
    }
    else if (error)
        return (cl_error);
    in_curlayer = in_layer;
    in_curdtype = in_dtype;
    if (changed)
        *changed = true;
    layer_tab->add(ll, new gds_lspec(in_layer, in_dtype, in_layer_name, 0),
        false);
    return (cl_ok);
}


// Specialization for cvOpenModeTrans
//
cl_type
gds_in::check_layer_cvt()
{
    bool lchg;
    cl_type cvt = check_layer(&lchg, false);
    if (cvt != cl_ok)
        return (cvt);
    if (lchg) {
        if (in_layers) {
            Layer layer(in_layers->ldesc->name(), in_curlayer, in_curdtype);
            if (!in_out->queue_layer(&layer))
                return (cl_error);
        }
        else {
            Layer layer(in_layer_name, in_curlayer, in_curdtype);
            if (!in_out->queue_layer(&layer))
                return (cl_error);
            char buf[256];
            if (!in_undef_layers)
                in_undef_layers = new SymTab(true, false);
            if (SymTab::get(in_undef_layers, in_layer_name) == ST_NIL) {
                sprintf(buf,
                    "no mapping for layer %d datatype %d (naming layer %s)",
                    in_curlayer, in_curdtype, in_layer_name);
                in_undef_layers->add(lstring::copy(in_layer_name), 0, false);
                warning(buf);
            }
        }
    }
    return (cl_ok);
}


// Add property list information
//
void
gds_in::add_properties_db(CDs *sdesc, CDo *odesc)
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


inline void
gds_in::clear_properties()
{
    while (in_prpty_list != 0) {
        CDp *pnext = in_prpty_list->next_prp();
        delete in_prpty_list;
        in_prpty_list = pnext;
    }
}


//
//===========================================================================
// Action functions
//===========================================================================
//

// The following two functions are universal

namespace {
    struct gdsrec
    {
        int type;
        const char *name;
    };

    // These are the V5 records that are not supported.
    gdsrec Unsup[] = {
        {II_PLEX,           "PLEX"},
        {II_TAPENUM,        "TAPENUM"},
        {II_TAPECODE,       "TAPECODE"},
        {II_STRCLASS,       "STRCLASS"},
        {II_FORMAT,         "FORMAT"},
        {II_MASK,           "MASK"},
        {II_ENDMASKS,       "ENDMASKS"},
        {II_LIBDIRSIZE,     "LIBDIRSIZE"},
        {II_SRFNAME,        "SRFNAME"},
        {II_LIBSECUR,       "LIBSECUR"},
        {II_BORDER,         "BORDER"},
        {II_SOFTFENCE,      "SOFTFENCE"},
        {II_HARDFENCE,      "HARDFENCE"},
        {II_SOFTWIRE,       "SOFTWIRE"},
        {II_HARDWIRE,       "HARDWIRE"},
        {II_PATHPORT,       "PATHPORT"},
        {II_NODEPORT,       "NODEPORT"},
        {II_USERCONSTRAINT, "USERCONSTRAINT"},
        {II_SPACER_ERROR,   "SPACER_ERROR"},
        {II_CONTACT,        "CONTACT"},
        {-1,                0}
    };
}


bool
gds_in::unsup()
{
    char buf[256];
    for (int i = 0; Unsup[i].name; i++) {
        if (in_rectype == Unsup[i].type) {
            if (in_action == cvOpenModePrint)
                fprintf(in_print_fp,
#ifdef WIN32
                    ">> Unsupported record type %s at offset %I64u\n",
#else
                    ">> Unsupported record type %s at offset %llu\n",
#endif
                    Unsup[i].name, (unsigned long long)in_offset);
            else {
                sprintf(buf, "unsupported record type %s", Unsup[i].name);
                warning(buf);
            }
            return (true);
        }
    }
    if (in_action == cvOpenModePrint)
#ifdef WIN32
        fprintf(in_print_fp, ">> Unknown record type %d at offset %I64u\n",
#else
        fprintf(in_print_fp, ">> Unknown record type %d at offset %llu\n",
#endif
            in_rectype, (unsigned long long)in_offset);
    else {
        sprintf(buf, "unknown record type %d", in_rectype);
        warning(buf);
    }
    return (true);
}


namespace {
    // These are the records that are ignored.
    gdsrec Ignored[] = {
        {II_TEXTNODE,       "TEXTNODE"},
        {II_SPACING,        "SPACING"},
        {II_UINTEGER,       "UINTEGER"},
        {II_USTRING,        "USTRING"},
        {II_STYPTABLE,      "STYPTABLE"},
        {II_STRTYPE,        "STRTYPE"},
        {II_ELFLAGS,        "ELFLAGS"},
        {II_ELKEY,          "ELKEY"},
        {II_LINKTYPE,       "LINKTYPE"},
        {II_LINKKEYS,       "LINKKEYS"},
        {II_NODETYPE,       "NODETYPE"},
        {-1,                0}
    };
}


bool
gds_in::nop()
{
    char buf[256];
    for (int i = 0; Ignored[i].name; i++) {
        if (in_rectype == Ignored[i].type) {
            if (in_action == cvOpenModePrint)
                fprintf(in_print_fp,
#ifdef WIN32
                    ">> Ignored record type %s at offset %I64u\n",
#else
                    ">> Ignored record type %s at offset %llu\n",
#endif
                    Ignored[i].name, (unsigned long long)in_offset);
            else {
                sprintf(buf, "ignored record type %s", Ignored[i].name);
                warning(buf);
            }
            return (true);
        }
    }
    if (in_action == cvOpenModePrint)
#ifdef WIN32
        fprintf(in_print_fp, ">> Ignored record type %d at offset %I64u\n",
#else
        fprintf(in_print_fp, ">> Ignored record type %d at offset %llu\n",
#endif
            in_rectype, (unsigned long long)in_offset);
    else {
        sprintf(buf, "ignored record type %d", in_rectype);
        warning(buf);
    }
    return (true);
}


//
//===========================================================================
// Actions for creating cells in database
//===========================================================================
//

bool
gds_in::a_header()
{
    if (in_mode == Electrical)
        return (true);
    if (in_ignore_prop)
        return (true);
    char buf1[64], buf2[64];
    in_version = shortval(in_cbuf);
    sprintf(buf1, "%d", in_version);
    sprintf(buf2, "( VERSION %d )", in_version);
    Attribute *h = new Attribute(GDSII_PROPERTY_BASE + II_HEADER, buf1, buf2);
    if (!in_sprops)
        in_sprops = h;
    else {
        CDp *hx = in_sprops;
        while (hx->next_prp())
            hx = hx->next_prp();
        hx->set_next_prp(h);
    }
    return (true);
}


bool
gds_in::a_bgnlib()
{
    return (true);
}


bool
gds_in::a_libname()
{
    if (in_mode == Electrical)
        return (true);
    char *tbf = new char[strlen(in_cbuf) + 16];
    sprintf(tbf, "( LIBNAME %s )", in_cbuf);
    Attribute *h =
        new Attribute(GDSII_PROPERTY_BASE + II_LIBNAME, in_cbuf, tbf);
    delete [] tbf;
    if (!in_sprops)
        in_sprops = h;
    else {
        CDp *hx = in_sprops;
        while (hx->next_prp())
            hx = hx->next_prp();
        hx->set_next_prp(h);
    }
    return (true);
}


bool
gds_in::a_units()
{
    in_munit = doubleval(in_cbuf+8);
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
    if (in_mode == Physical && in_munit != 1e-9) {
        double uunit = doubleval(in_cbuf);
        FIO()->ifPrintCvLog(IFLOG_INFO, "Units: m_unit=%.5e  u_unit=%.5e",
            in_munit, uunit);
    }
    return (true);
}


bool
gds_in::a_bgnstr()
{
    in_cell_offset = in_offset;
    char *p = in_cbuf;
    date_value(&in_cdate, &p);
    date_value(&in_mdate, &p);

    in_obj_offset = 0;
    in_attrib = -1;
    reset_layer_check();
    in_sdesc = 0;
    CD()->InitCoincCheck();

    if (!get_record())
        return (false);
    if (in_rectype != II_STRNAME) {
        Errs()->add_error("Unexpected record type %d.", in_rectype);
        fatal_error();
        return (false);
    }
    if (!a_strname())
        return (false);

    if (in_savebb)
        in_cBB = CDnullBB;

    bool dup_sym = false;
    symref_t *srf = 0;
    if (in_uselist) {
        // CHD name might be aliased or not.
        if (in_chd_state.symref())
            srf = in_chd_state.symref();
        else {
            srf = get_symref(in_cbuf, in_mode);
            if (!srf)
                srf = get_symref(in_cellname, in_mode);
            if (!srf) {
                Errs()->add_error("Can't find %s in CHD.", in_cbuf);
                fatal_error();
                return (false);
            }
        }
    }
    else {
        nametab_t *ntab = get_sym_tab(in_mode);
        srf = get_symref(in_cbuf, in_mode);
        if (srf && srf->get_defseen()) {
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "Duplicate cell definition for %s at offset %llu.",
                in_cellname, (unsigned long long)in_offset);
            dup_sym = true;
        }

        if (!srf) {
            ntab->new_symref(in_cbuf, in_mode, &srf);
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

        if (in_attr_offset)
            srf->set_offset(in_attr_offset);
        else if (in_cell_offset)
            srf->set_offset(in_cell_offset);
        else {
            Errs()->add_error("Internal error: a_bgnstr(), no offset.");
            fatal_error();
            return (false);
        }
    }
    if (info())
        info()->add_cell(srf);

    if (!in_listonly) {
        CDs *sd = CDcdb()->findCell(in_cbuf, in_mode);
        if (!sd) {
            // doesn't already exist, open it
            sd = CDcdb()->insertCell(in_cbuf, in_mode);
            sd->setFileType(Fgds);
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
                    in_cbuf);
            }
        }
        else if (sd->isLibrary() && sd->isDevice()) {
            const char *nname = handle_lib_clash(in_cbuf);
            if (nname)
                strcpy(in_cbuf, nname);
        }
        else if (in_mode == Physical) {
            bool reuse = false;
            if (sd->isUnread()) {
                sd->setUnread(false);
                reuse = true;
            }
            else {
                mitem_t mi(in_cbuf);
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
                sd->setFileType(Fgds);
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

                    mitem_t mi(in_cbuf);
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
                sd->setFileType(Fgds);
                sd->setFileName(in_filename);
                if (in_lcheck || in_layer_alias || in_phys_scale != 1.0)
                    sd->setAltered(true);
                in_sdesc = sd;
            }
        }

        in_prpty_list = in_eprops;
        in_eprops = 0;
        if (in_sdesc) {
            // add the electrical property list and clear
            FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
                in_mode);
            add_properties_db(in_sdesc, 0);
        }
    }
    clear_properties();

    uint64_t symoff = in_offset;

    bool nbad;
    while ((nbad = get_record())) {
        if (in_rectype == II_ENDSTR)
            break;

        in_obj_offset = in_offset;
        if (in_rectype == II_EOF) {
            Errs()->add_error("Premature end of file.");
            fatal_error();
            return (false);
        }
        if (in_rectype >= GDS_NUM_REC_TYPES) {
            unsup();
            continue;
        }
        if (!(this->*ftab[in_rectype])())
            return (false);
    }
    if (!nbad)
        return (false);

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
    else if (in_sdesc) {
        if (in_mode == Physical) {
            if (!in_savebb && !in_no_test_empties && in_sdesc->isEmpty()) {
                FIO()->ifPrintCvLog(IFLOG_INFO,
                    "Cell %s physical definition is empty at offset %llu.",
                    in_cellname, (unsigned long long)symoff);
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

    in_attr_offset = 0;
    in_cell_offset = 0;
    return (true);
}


bool
gds_in::a_strname()
{
    // in_cellname is unaliased
    strcpy(in_cellname, in_cbuf);
    if (in_chd_state.symref())
        strcpy(in_cbuf, Tstring(in_chd_state.symref()->get_name()));
    else
        strcpy(in_cbuf, alias(in_cellname));
    return (true);
}


bool
gds_in::a_boundary()
{
    if (!read_element())
        return (false);
    cl_type cvt = check_layer();
    if (cvt == cl_error)
        return (false);
    if (cvt == cl_skip || (in_listonly && !in_savebb)) {
        clear_properties();
        return (true);
    }

    if (in_sdesc || in_savebb) {

        bool do_rect = false;
        if (in_numpts == 5 && ((in_points[0].x == in_points[1].x &&
                in_points[1].y == in_points[2].y &&
                in_points[2].x == in_points[3].x &&
                in_points[3].y == in_points[0].y) ||
                (in_points[0].y == in_points[1].y &&
                in_points[1].x == in_points[2].x &&
                in_points[2].y == in_points[3].y &&
                in_points[3].x == in_points[0].x)))
            do_rect = true;

        if (do_rect) {
            if (info())
                info()->add_box(1);
            BBox BB(in_points);

            if (in_savebb)
                in_cBB.add(&BB);
            else if (!in_areafilt || BB.intersect(&in_cBB, false)) {
                for (CDll *ll = in_layers; ll; ll = ll->next) {
                    CDo *newo;
                    CDerrType err = in_sdesc->makeBox(ll->ldesc, &BB, &newo);
                    if (err != CDok) {
                        if (err == CDbadBox) {
                            warning("bad rectangular polygon (ignored)",
                                BB.left, BB.bottom);
                            break;
                        }
                        clear_properties();
                        return (false);
                    }
                    if (newo) {
                        newo->set_flag(
                            ll->ldesc->getStrmDatatypeFlags(in_curdtype));
                        add_properties_db(in_sdesc, newo);
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
        else {
            if (info())
                info()->add_poly(in_numpts);
            Poly poly(in_numpts, in_points);
            if (in_savebb) {
                BBox BB;
                poly.computeBB(&BB);
                in_cBB.add(&BB);
            }
            else if (!in_areafilt || poly.intersect(&in_cBB, false)) {
                for (CDll *ll = in_layers; ll; ll = ll->next) {
                    poly.points = Point::dup(in_points, in_numpts);
                    CDpo *newo;
                    int pchk_flags;
                    CDerrType err = in_sdesc->makePolygon(ll->ldesc, &poly,
                        &newo, &pchk_flags);
                    if (err != CDok) {
                        if (err == CDbadPolygon) {
                            warning("bad polygon (ignored)",
                                in_points->x, in_points->y);
                            break;
                        }
                        clear_properties();
                        return (false);
                    }
                    if (newo) {
                        if (ll == in_layers) {
                            if (pchk_flags & PCHK_REENT)
                                warning("badly formed polygon (kept)",
                                    in_points->x, in_points->y);
                            if (pchk_flags & PCHK_ZERANG) {
                                if (pchk_flags & PCHK_FIXED)
                                    warning("repaired polygon 0/180 angle",
                                        in_points->x, in_points->y);
                                else
                                    warning("polygon with 0/180 angle",
                                        in_points->x, in_points->y);
                            }
                        }
                        newo->set_flag(
                            ll->ldesc->getStrmDatatypeFlags(in_curdtype));
                        add_properties_db(in_sdesc, newo);
                    }
                }
            }
        }
    }
    clear_properties();
    return (true);
}


bool
gds_in::a_path()
{
    if (!read_element())
        return (false);
    cl_type cvt = check_layer();
    if (cvt == cl_error)
        return (false);
    if (cvt == cl_skip || (in_listonly && !in_savebb)) {
        clear_properties();
        return (true);
    }

    if (info())
        info()->add_wire(in_numpts);
    int bgnextn = in_bextn;
    int endextn = in_eextn;
    if (in_ptype == 4) {
        in_ptype4_cnt++;
        convert_4to0(&in_points[0].x, &in_points[0].y, in_points[1].x,
            in_points[1].y, bgnextn);
        convert_4to0(&in_points[in_numpts-1].x, &in_points[in_numpts-1].y,
            in_points[in_numpts-2].x, in_points[in_numpts-2].y, endextn);
        in_ptype = 0;
    }
    else if (in_ptype != 0 && in_ptype != 1 && in_ptype != 2) {
        sprintf(in_cbuf, "unknown pathtype %d set to 0", in_ptype);
        warning(in_cbuf, in_points->x, in_points->y);
        in_ptype = 0;
    }
    if (in_pwidth < 0) {
        warning("unsupported absolute path width taken as relative",
            in_points->x, in_points->y);
        in_pwidth = -in_pwidth;
    }
    if (in_sdesc || in_savebb) {
        Wire wire(in_pwidth, (WireStyle)in_ptype, in_numpts, in_points);
        if (in_savebb) {
            BBox BB;
            // warning: if dup verts, computeBB may fail
            Point::removeDups(wire.points, &wire.numpts);
            wire.computeBB(&BB);
            in_cBB.add(&BB);
        }
        else if (!in_areafilt || wire.intersect(&in_cBB, false)) {
            for (CDll *ll = in_layers; ll; ll = ll->next) {
                wire.points = new Point[in_numpts];
                for (int i = 0; i < in_numpts; i++)
                    wire.points[i] = in_points[i];

                Point *pres = 0;
                int nres = 0;
                for (;;) {
                    if (in_mode == Physical)
                        wire.checkWireVerts(&pres, &nres);

                    CDw *newo;
                    int wchk_flags;
                    CDerrType err = in_sdesc->makeWire(ll->ldesc, &wire, &newo,
                        &wchk_flags);
                    if (err != CDok) {
                        if (err == CDbadWire) {
                            warning("bad wire (ignored)",
                                in_points->x, in_points->y);
                        }
                        else {
                            delete [] pres;
                            clear_properties();
                            return (false);
                        }
                    }
                    else if (newo) {
                        if (wchk_flags && ll == in_layers) {
                            char buf[256];
                            Wire::flagWarnings(buf, wchk_flags,
                                "questionable wire, warnings: ");
                            warning(buf, in_points->x, in_points->y);
                        }
                        newo->set_flag(
                            ll->ldesc->getStrmDatatypeFlags(in_curdtype));
                        add_properties_db(in_sdesc, newo);
                    }
                    if (!pres)
                        break;
                    warning("breaking colinear reentrant wire",
                        in_points->x, in_points->y);
                    wire.points = pres;
                    wire.numpts = nres;
                }
            }
        }
    }
    clear_properties();
    return (true);
}


bool
gds_in::a_sref()
{
    return (a_instance(false));
}


bool
gds_in::a_aref()
{
    return (a_instance(true));
}


bool
gds_in::a_instance(bool ary)
{
    if (!read_element())
        return (false);

    if (in_chd_state.gen()) {
        delete [] in_string;
        in_string = 0;
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
            Errs()->add_error(
                "Unresolved transform ticket %d.", c->attr);
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
        if (ap.dx == 0 && ap.nx > 1)
            ap.nx = 1;
        if (ap.dy == 0 && ap.ny > 1)
            ap.ny = 1;

        cname = check_sub_master(cname);
        CallDesc calldesc(cname, 0);

        CDc *newo;
        if (OIfailed(in_sdesc->makeCall(&calldesc, &tx, &ap, CDcallDb, &newo)))
            return (false);
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0, in_mode);
        add_properties_db(in_sdesc, newo);
        clear_properties();
        return (true);
    }

    if (info())
        info()->add_inst(ary);

    if (!in_ignore_inst && (in_sdesc || (in_listonly && in_symref))) {
        const char *cellname = alias(in_string);

        int ax, ay;
        char *emsg = FIO()->GdsParamSet(&in_angle, &in_magn, &ax, &ay);
        if (emsg) {
            warning(emsg, cellname, in_points->x, in_points->y);
            delete [] emsg;
        }

        CDap ap;
        if (ary) {
            if (!in_nx)
                in_nx = 1;
            if (!in_ny)
                in_ny = 1;
            // The pts are three coordinates of transformed points of the
            // box:
            //   in_points[0] (PREF)  0, 0
            //   in_points[1] (PCOL)  dx, 0
            //   in_points[2] (PROW)  0, dy
            // Apply the inverse transform to get dx, dy.
            //
            Point tp[3];
            tp[0] = in_points[0];
            tp[1] = in_points[1];
            tp[2] = in_points[2];
            TPush();
            // defer de-magnification until after difference
            TApply(tp[0].x, tp[0].y, ax, ay, 1.0, in_reflection);
            TInverse();
            TLoadInverse();
            TPoint(&tp[0].x, &tp[0].y);
            TPoint(&tp[1].x, &tp[1].y);
            TPoint(&tp[2].x, &tp[2].y);
            TPop();
            ap.nx = in_nx;
            ap.ny = in_ny;
            ap.dx = mmRnd((tp[1].x - tp[0].x)/(in_nx*in_magn));
            ap.dy = mmRnd((tp[2].y - tp[0].y)/(in_ny*in_magn));
            if (ap.dx == 0 && ap.nx > 1)
                ap.nx = 1;
            if (ap.dy == 0 && ap.ny > 1)
                ap.ny = 1;
        }
        if (in_listonly && in_symref) {
            nametab_t *ntab = get_sym_tab(in_mode);
            cref_t *ar;
            ticket_t ctk = ntab->new_cref(&ar);
#ifdef VIAS_IN_LISTONLY
            if (in_mode == Physical) {
                CDcellName cname = CD()->CellNameTableAdd(cellname);
                cname = check_sub_master(cname);
                cellname = Tstring(cname);
            }
#endif
            symref_t *ptr = get_symref(cellname, in_mode);
            if (!ptr) {
                ar->set_refptr(ntab->new_symref(cellname, in_mode, &ptr));
                add_symref(ptr, in_mode);
            }
            else
                ar->set_refptr(ptr->get_ticket());

            CDtx tx(in_reflection, ax, ay, in_points->x, in_points->y,
                in_magn);
            ar->set_pos_x(tx.tx);
            ar->set_pos_y(tx.ty);
            CDattr at(&tx, &ap);
            ar->set_tkt(CD()->RecordAttr(&at));

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

            CDtx tx(in_reflection, ax, ay, in_points->x, in_points->y,
                in_magn);
            CDc *newo;
            if (OIfailed(in_sdesc->makeCall(&calldesc, &tx, &ap, CDcallDb,
                    &newo)))
                return (false);
            FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
                in_mode);
            add_properties_db(in_sdesc, newo);

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
    delete [] in_string;
    in_string = 0;
    clear_properties();
    return (true);
}


inline int
xform_rot_bits(int ax, int ay)
{
    // xform bits:
    // 0-1, 0-no rotation, 1-90, 2-180, 3-270.
    // 2, mirror y after rotation
    // 3, mirror x after rotation and mirror y
    // ---- above are legacy ----
    // 4, shift rotation to 45, 135, 225, 315
    // 5-6 horiz justification 00,11 left, 01 center, 10 right
    // 7-8 vert justification 00,11 bottom, 01 center, 10 top
    // 9-10 font
    //
    int xform = 0;
    if (!ax && ay) {
        if (ay > 0)
            xform = 1;        // 90
        else
            xform = 3;        // 270
    }
    else if (ax && !ay) {
        if (ax < 0)
            xform = 2;        // 180
    }
    else {
        if (ax > 0) {
            if (ay < 0)
                xform = 3;    // 315
        }
        else {
            if (ay > 0)
                xform = 1;    // 135
            else
                xform = 2;    // 225
        }
        xform |= TXTF_45;     // shift
    }
    return (xform);
}


bool
gds_in::a_text()
{
    if (!read_element())
        return (false);
    bool ret = true;
    if (in_mode == Physical && in_layer == 0 && in_dtype == 0 &&
            !strcmp(in_string, GDS_CELL_PROPERTY_MAGIC)) {

        // This is no longer written into Xic output, instead the
        // properties are saved in a SNAPNODE.  This is kept for
        // backwards compatibility.

        // Since GDSII does not support cell properties, in physical
        // mode the cell properties, if any, are saved on a dummy
        // label.  In electrical mode, cell properties are saved using
        // a format extension.  The extension would horribly break
        // portability if used in physical mode.

        // Add the properties to the cell, ignore the label.
        add_properties_db(in_sdesc, 0);
        delete [] in_string;
        in_string = 0;
        clear_properties();
        return (true);
    }
    if (!in_ignore_text) {
        cl_type cvt = check_layer();
        if (cvt == cl_error)
            return (false);
        if (cvt == cl_skip || (in_listonly && !in_savebb)) {
            delete [] in_string;
            in_string = 0;
            clear_properties();
            return (true);
        }
        if (info())
            info()->add_text();

        double oldang = in_angle;
        int ax, ay;
        char *emsg = FIO()->GdsParamSet(&in_angle, &in_magn, &ax, &ay);
        if (emsg) {
            warning(emsg, in_points->x, in_points->y);
            delete [] emsg;
        }
        else
            oldang = in_angle;

        int xform = xform_rot_bits(ax, ay);
        if (in_reflection)
            xform |= TXTF_MY;  // mirror about x axis, before rotation
        // horizontal justification
        if (in_presentation & 0x1)
            xform |= TXTF_HJC;
        else if (in_presentation & 0x2)
            xform |= TXTF_HJR;
        // vertical justification
        if (in_presentation & 0x4)
            xform |= TXTF_VJC;
        else if (!(in_presentation & 0x8))
            xform |= TXTF_VJT;
        // Note that if vertical justification bits are *not* set, top
        // justification is indicated.
        int ft = (in_presentation & 0x30) >> 4; // font
        TXTF_SET_FONT(xform, ft);

        if (in_magn < 0.1)
            in_magn = 0.1;

        // in_sdesc can be 0
        hyList *hpl = new hyList(in_sdesc, in_string, HYcvAscii);

        int wid, hei;
        if (in_text_width > 0 && in_text_height > 0) {
            // Size set from Xic property.
            wid = in_text_width;
            hei = in_text_height;
            in_text_width = 0;
            in_text_height = 0;
            if (in_needs_mult) {
                wid = scale(wid);
                hei = scale(hei);
            }
            xform |= (in_text_flags &
                (TXTF_SHOW | TXTF_HIDE | TXTF_TLEV | TXTF_LIML));
            in_text_flags = 0;
        }
        else {
            // This is the displayed string, not necessarily the same as the
            // label text.
            char *string = hyList::string(hpl, HYcvPlain, false);
            double tw, th;
            CD()->DefaultLabelSize(string, in_mode, &tw, &th);
            delete [] string;

            // The default size implies CDxxxResolution counts per
            // micron.  We have to account for different resolution in
            // the GDSII file.
            if (in_mode == Physical) {
                wid = INTERNAL_UNITS(tw*in_ext_phys_scale*in_magn);
                hei = INTERNAL_UNITS(th*in_ext_phys_scale*in_magn);
            }
            else {
                wid = ELEC_INTERNAL_UNITS(tw*in_magn);
                hei = ELEC_INTERNAL_UNITS(th*in_magn);
            }
        }

        if (in_sdesc || in_savebb) {
            Label label(0, in_points->x, in_points->y, wid, hei, xform);
            if (in_savebb) {
                BBox BB;
                label.computeBB(&BB);
                in_cBB.add(&BB);
            }
            else if (!in_areafilt || label.intersect(&in_cBB, false)) {
                // String to store stream stuff we don't use
                if (in_mode == Physical && (oldang != 0.0 || in_magn != 1.0 ||
                        in_pwidth || in_ptype)) {
                    char buf[256];
                    *buf = 0;
                    char *s = buf;
                    if (oldang != 0.0) {
                        s = lstring::stpcpy(s, " ANGLE ");
                        s = mmDtoA(s, oldang, 6);
                    }
                    if (in_magn != 1.0) {
                        s = lstring::stpcpy(s, " MAG ");
                        s = mmDtoA(s, in_magn, 6);
                    }
                    if (in_pwidth) {
                        s = lstring::stpcpy(s, " WIDTH ");
                        s = mmItoA(s, in_pwidth);
                    }
                    if (in_ptype) {
                        s = lstring::stpcpy(s, " PTYPE ");
                        s = mmItoA(s, in_ptype);
                    }
                    if (*buf) {
                        // add a property for the stream info
                        CDp *pd = new CDp(buf+1,
                            II_TEXT + GDSII_PROPERTY_BASE);
                        pd->set_next_prp(in_prpty_list);
                        in_prpty_list = pd;
                    }
                }
                for (CDll *ll = in_layers; ll; ll = ll->next) {
                    label.label = hyList::dup(hpl);
                    CDla *newo;
                    CDerrType err = in_sdesc->makeLabel(ll->ldesc, &label,
                        &newo);
                    if (err != CDok) {
                        if (err == CDbadLabel) {
                            warning("bad label (ignored)",
                                in_points->x, in_points->y);
                            break;
                        }
                        ret = false;
                        break;
                    }
                    if (newo)
                        add_properties_db(in_sdesc, newo);
                }
            }
        }
        hyList::destroy(hpl);
    }
    delete [] in_string;
    in_string = 0;
    clear_properties();
    return (ret);
}


bool
gds_in::a_layer()
{
    in_layer = (unsigned short)shortval(in_cbuf);
    return (true);
}


bool
gds_in::a_datatype()
{
    in_dtype = (unsigned short)shortval(in_cbuf);
    return (true);
}


bool
gds_in::a_width()
{
    in_pwidth = scale(longval(in_cbuf));
    return (true);
}


bool
gds_in::a_xy()
{
    return (true);
}


bool
gds_in::a_sname()
{
    in_string = lstring::copy(in_cbuf);
    return (true);
}


bool
gds_in::a_colrow()
{
    in_nx = (unsigned short)shortval(in_cbuf);
    in_ny = (unsigned short)shortval(in_cbuf+2);
    return (true);
}


bool
gds_in::a_snapnode()
{
    if (!read_element())
        return (false);

    if (in_mode == Physical && in_layer == 0 && in_dtype == 0 &&
            in_plexnum == XIC_NODE_PLEXNUM) {

        // Since GDSII does not support cell properties, in physical
        // mode the cell properties, if any, are saved on a dummy
        // node.  In electrical mode, cell properties are saved using
        // a format extension.  The extension would horribly break
        // portability if used in physical mode.

        // Add the properties to the cell, ignore the node.
        add_properties_db(in_sdesc, 0);
    }

    clear_properties();
    return (true);
}


bool
gds_in::a_presentation()
{
    in_presentation = shortval(in_cbuf);
    return (true);
}


bool
gds_in::a_string()
{
    in_string = lstring::copy(in_cbuf);
    return (true);
}


bool
gds_in::a_strans()
{
    if (in_cbuf[1] & 4)
        warning("unsupported absolute magnification taken as relative");
    if (in_cbuf[1] & 2)
        warning("unsupported absolute angle taken as relative");
    if (in_cbuf[0] & 128)
        in_reflection = true;
    else
        in_reflection = false;
    return (true);
}


bool
gds_in::a_mag()
{
    in_magn = doubleval(in_cbuf);
    return (true);
}


bool
gds_in::a_angle()
{
    in_angle = doubleval(in_cbuf);
    return (true);
}


bool
gds_in::a_reflibs()
{
    if (in_mode == Electrical)
        return (true);
    if (in_ignore_prop)
        return (true);
    char buf1[256], buf2[256];
    int i;
    for (i = 0; i < GDS_MAX_LIBNAM_LEN && in_cbuf[i]; i++)
        buf1[i] = in_cbuf[i];
    buf1[i++] = ' ';
    char *cbp = in_cbuf + GDS_MAX_LIBNAM_LEN;
    for (int j = 0; j < GDS_MAX_LIBNAM_LEN && cbp[j]; j++)
        buf1[i++] = cbp[j];
    buf1[i++] = '\0';
    strcpy(buf2, "( REFLIBS )");
    Attribute *h = new Attribute(GDSII_PROPERTY_BASE + II_REFLIBS, buf1, buf2);
    if (!in_sprops)
        in_sprops = h;
    else {
        CDp *hx = in_sprops;
        while (hx->next_prp())
            hx = hx->next_prp();
        hx->set_next_prp(h);
    }
    return (true);
}


bool
gds_in::a_fonts()
{
    if (in_mode == Electrical)
        return (true);
    if (in_ignore_prop)
        return (true);
    char buf1[256], buf2[256];
    int i;
    for (i = 0; i < GDS_MAX_LIBNAM_LEN && in_cbuf[i]; i++)
        buf1[i] = in_cbuf[i];
    buf1[i++] = ' ';
    char *cbp = in_cbuf + GDS_MAX_LIBNAM_LEN;
    for (int j = 0; j < GDS_MAX_LIBNAM_LEN && cbp[j]; j++)
        buf1[i++] = cbp[j];
    buf1[i++] = ' ';
    cbp = in_cbuf + 2*GDS_MAX_LIBNAM_LEN;
    for (int j = 0; j < GDS_MAX_LIBNAM_LEN && cbp[j]; j++)
        buf1[i++] = cbp[j];
    buf1[i++] = ' ';
    cbp = in_cbuf + 3*GDS_MAX_LIBNAM_LEN;
    for (int j = 0; j < GDS_MAX_LIBNAM_LEN && cbp[j]; j++)
        buf1[i++] = cbp[j];
    buf1[i++] = '\0';
    strcpy(buf2, "( FONTS )");
    Attribute *h = new Attribute(GDSII_PROPERTY_BASE + II_FONTS, buf1, buf2);
    if (!in_sprops)
        in_sprops = h;
    else {
        CDp *hx = in_sprops;
        while (hx->next_prp())
            hx = hx->next_prp();
        hx->set_next_prp(h);
    }
    return (true);
}


bool
gds_in::a_pathtype()
{
    // STREAM pathtypes
    // 0 = square ended paths with ends that are flush
    //     with the endpoints
    // 1 = round ended (CIF like) paths
    // 2 = square ended paths with ends that are
    //     offset from the endpoint by a half width.
    // V5 feature:
    // 4 = square ended paths with ends that extend past the
    //     endpoints by factors given in BGNEXTN and ENDEXTN.
    //
    in_ptype = shortval(in_cbuf);
    return (true);
}


bool
gds_in::a_generations()
{
    if (in_mode == Electrical)
        return (true);
    if (in_ignore_prop)
        return (true);
    char buf1[256], buf2[256];
    sprintf(buf1, "%d", shortval(in_cbuf));
    sprintf(buf2, "( GENERATIONS %d )", shortval(in_cbuf));
    Attribute *h = new Attribute(GDSII_PROPERTY_BASE + II_GENERATIONS, buf1,
        buf2);
    if (!in_sprops)
        in_sprops = h;
    else {
        CDp *hx = in_sprops;
        while (hx->next_prp())
            hx = hx->next_prp();
        hx->set_next_prp(h);
    }
    return (true);
}


bool
gds_in::a_attrtable()
{
    if (in_mode == Electrical)
        return (true);
    if (in_ignore_prop)
        return (true);
    char *tbf = new char[strlen(in_cbuf) + 22];
    sprintf(tbf, "( ATTRIBUTE TABLE %s )", in_cbuf);
    Attribute *h =
        new Attribute(GDSII_PROPERTY_BASE + II_ATTRTABLE, in_cbuf, tbf);
    delete [] tbf;
    if (!in_sprops)
        in_sprops = h;
    else {
        CDp *hx = in_sprops;
        while (hx->next_prp())
            hx = hx->next_prp();
        hx->set_next_prp(h);
    }
    return (true);
}


bool
gds_in::a_propattr()
{
    if (!in_elemrec) {
        // cell property extension
        if (in_attr_offset == 0)
            in_attr_offset = in_offset;
    }
    in_attrib = shortval(in_cbuf);
    return (true);
}


bool
gds_in::a_propvalue()
{
    if ((in_attrib == XICP_GDS_LABEL_SIZE ||
            in_attrib == OLD_GDS_LABEL_SIZE_PROP) && in_elemrec) {
        if (read_text_property())
            return (true);
    }
    if (in_ignore_prop)
        return (true);
    if (!in_elemrec) {
        // cell property extension
        if (prpty_gdsii(in_attrib)) {
            in_attrib = -1;
            return (true);
        }
        char buf2[256];
        strcpy(buf2, "( electrical property )");
        Attribute *h = new Attribute(in_attrib, in_cbuf, buf2);
        h->set_next_prp(in_eprops);
        in_eprops = h;
    }
    else {
        CDp *p = new CDp(in_cbuf, in_attrib);
        p->set_next_prp(in_prpty_list);
        in_prpty_list = p;
    }
    in_attrib = -1;
    return (true);
}


bool
gds_in::a_box()
{
    return (a_boundary());
}


bool
gds_in::a_plex()
{
    in_plexnum = longval(in_cbuf);
    return (true);
}


bool
gds_in::a_bgnextn()
{
    in_bextn = scale(longval(in_cbuf));
    return (true);
}


bool
gds_in::a_endextn()
{
    in_eextn = scale(longval(in_cbuf));
    return (true);
}


//
//===========================================================================
// Actions for format translation
//===========================================================================
//

// These replace the corresponding functions above.

bool
gds_in::ac_bgnstr()
{
    in_cell_offset = in_offset;
    char *p = in_cbuf;
    date_value(&in_cdate, &p);
    date_value(&in_mdate, &p);

    if (!get_record())
        return (false);
    if (in_rectype != II_STRNAME) {
        Errs()->add_error("Unexpected record type %d.", in_rectype);
        fatal_error();
        return (false);
    }

    char cbuf_bak[256];
    bool firstrec = (in_mode == Physical);
    if (in_transform <= 0) {
        if (!a_strname())
            return (false);

        in_obj_offset = 0;
        in_attrib = -1;
        reset_layer_check();
        if (info())
            info()->add_cell(in_chd_state.symref());
        if (firstrec)
            // save this, it contains cell name
            strcpy(cbuf_bak, in_cbuf);
        else {
            if (!in_out->write_struct(in_cbuf, &in_cdate, &in_mdate))
                return (false);
            in_out->clear_property_queue();
        }
    }

    bool nbad;
    while ((nbad = get_record())) {

        // If the file was produced by Xic, in physical mode the first
        // element record is a dummy SNAPNODE that has the cell
        // properties attached.  We defer streaming the cell header
        // until this has been read, so we can attach the cell
        // properties on output.  Older releases put the properties on
        // a label, so we handle that, too.

        if (firstrec) {
            firstrec = false;
            if (in_rectype == II_SNAPNODE) {
                if (!(this->*ftab[in_rectype])())
                    return (false);
                if (in_layer != 0 || in_dtype != 0 ||
                        in_plexnum != XIC_NODE_PLEXNUM)
                    // not ours, ignore properties
                    in_out->clear_property_queue();
                if (in_transform <= 0) {
                    if (!in_out->write_struct(cbuf_bak, &in_cdate, &in_mdate))
                        return (false);
                }
                in_out->clear_property_queue();
                continue;
            }
            if (in_rectype == II_TEXT) {
                if (!read_element())
                    return (false);
                if (in_layer != 0 || in_dtype != 0 ||
                        strcmp(in_string, GDS_CELL_PROPERTY_MAGIC)) {
                    // not ours, ignore properties
                    CDp *ptmp = in_out->set_property_queue(0);
                    if (in_transform <= 0) {
                        if (!in_out->write_struct(cbuf_bak, &in_cdate,
                                &in_mdate))
                            return (false);
                    }
                    in_out->set_property_queue(ptmp);
                    if (!ac_text_core())
                        return (false);
                }
                else {
                    delete [] in_string;
                    in_string = 0;
                    if (in_transform <= 0) {
                        if (!in_out->write_struct(cbuf_bak, &in_cdate,
                                &in_mdate))
                            return (false);
                    }
                }
                in_out->clear_property_queue();
                continue;
            }

            if (in_transform <= 0) {
                if (!in_out->write_struct(cbuf_bak, &in_cdate, &in_mdate))
                    return (false);
                // This should be empty.
                in_out->clear_property_queue();
            }
        }

        if (in_rectype == II_ENDSTR)
            break;
        in_obj_offset = in_offset;
        if (in_rectype == II_EOF) {
            Errs()->add_error("Premature end of file.");
            fatal_error();
            return (false);
        }
        if (in_rectype >= GDS_NUM_REC_TYPES) {
            unsup();
            continue;
        }
        if (!(this->*ftab[in_rectype])())
            return (false);
    }
    if (!nbad)
        return (false);

    if (in_transform <= 0) {
        if (!in_flatten) {
            if (!in_out->write_end_struct())
                return (false);
        }
        in_attr_offset = 0;
        in_cell_offset = 0;
    }

    return (true);
}


bool
gds_in::ac_boundary()
{
    if (!read_element())
        return (false);
    cl_type cvt = check_layer_cvt();
    if (cvt == cl_error)
        return (false);
    if (cvt == cl_skip) {
        in_out->clear_property_queue();
        return (true);
    }
    if (in_points[0] != in_points[in_numpts - 1]) {
        in_points[in_numpts] = in_points[0];
        in_numpts++;
        warning("polygon without closure point (fixed)",
            in_points->x, in_points->y);
    }

    bool ret = true;
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        Point *scratch = new Point[in_numpts];
        memcpy(scratch, in_points, in_numpts*sizeof(Point));

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
                        memcpy(in_points, scratch, in_numpts*sizeof(Point));
                    ret = ac_boundary_prv();
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once)
                    memcpy(in_points, scratch, in_numpts*sizeof(Point));
                ret = ac_boundary_prv();
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
        ret = ac_boundary_prv();

    in_out->clear_property_queue();
    return (ret);
}


bool
gds_in::ac_boundary_prv()
{
    if (in_transform)
        TPath(in_numpts, in_points);
    bool do_rect = false;
    if (in_numpts == 5 && ((in_points[0].x == in_points[1].x &&
            in_points[1].y == in_points[2].y &&
            in_points[2].x == in_points[3].x &&
            in_points[3].y == in_points[0].y) ||
            (in_points[0].y == in_points[1].y &&
            in_points[1].x == in_points[2].x &&
            in_points[2].y == in_points[3].y &&
            in_points[3].x == in_points[0].x)))
        do_rect = true;

    bool ret = true;
    if (do_rect) {
        if (info())
            info()->add_box(1);
        BBox BB(in_points);

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
        if (!in_areafilt || BB.intersect(&in_cBB, false)) {
            TPush();
            for (CDll *ll = in_layers; ; ) {
                ret = in_out->write_box(&BB);
                if (!ret)
                    break;
                if (!ll)
                    break;
                ll = ll->next;
                if (!ll)
                    break;
                Layer layer(ll->ldesc->name(), in_curlayer, in_curdtype);
                ret = in_out->queue_layer(&layer);
                if (!ret)
                    break;
            }
            TPop();
        }
    }
    else {
        if (info())
            info()->add_poly(in_numpts);
        Poly po(in_numpts, in_points);
        if (!in_areafilt || po.intersect(&in_cBB, false)) {
            TPush();
            bool need_out = true;
            if (in_areafilt && in_clip) {
                need_out = false;
                PolyList *pl = po.clip(&in_cBB, &need_out);
                for (PolyList *p = pl; p; p = p->next) {
                    for (CDll *ll = in_layers; ; ) {
                        ret = in_out->write_poly(&p->po);
                        if (!ret)
                            break;
                        if (!ll)
                            break;
                        ll = ll->next;
                        if (!ll)
                            break;
                        Layer layer(ll->ldesc->name(), in_curlayer,
                            in_curdtype);
                        ret = in_out->queue_layer(&layer);
                        if (!ret)
                            break;
                    }
                    if (!ret)
                        break;
                }
                PolyList::destroy(pl);
            }
            if (need_out) {
                for (CDll *ll = in_layers; ; ) {
                    ret = in_out->write_poly(&po);
                    if (!ret)
                        break;
                    if (!ll)
                        break;
                    ll = ll->next;
                    if (!ll)
                        break;
                    Layer layer(ll->ldesc->name(), in_curlayer,
                        in_curdtype);
                    ret = in_out->queue_layer(&layer);
                    if (!ret)
                        break;
                }
            }
            TPop();
        }
    }
    return (ret);
}


bool
gds_in::ac_path()
{
    if (!read_element())
        return (false);
    cl_type cvt = check_layer_cvt();
    if (cvt == cl_error)
        return (false);
    if (cvt == cl_skip) {
        in_out->clear_property_queue();
        return (true);
    }

    if (info())
        info()->add_wire(in_numpts);
    int bgnextn = in_bextn;
    int endextn = in_eextn;
    if (in_ptype == 4) {
        in_ptype4_cnt++;
        convert_4to0(&in_points[0].x, &in_points[0].y, in_points[1].x,
            in_points[1].y, bgnextn);
        convert_4to0(&in_points[in_numpts-1].x, &in_points[in_numpts-1].y,
            in_points[in_numpts-2].x, in_points[in_numpts-2].y, endextn);
        in_ptype = 0;
    }
    else if (in_ptype != 0 && in_ptype != 1 && in_ptype != 2) {
        sprintf(in_cbuf, "unknown pathtype %d set to 0", in_ptype);
        warning(in_cbuf, in_points->x, in_points->y);
        in_ptype = 0;
    }
    if (in_pwidth < 0) {
        warning("unsupported absolute path width taken as relative",
            in_points->x, in_points->y);
        in_pwidth = -in_pwidth;
    }
    Point::removeDups(in_points, &in_numpts);

    bool ret = true;
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        Point *scratch = new Point[in_numpts];
        memcpy(scratch, in_points, in_numpts*sizeof(Point));

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
                        memcpy(in_points, scratch, in_numpts*sizeof(Point));
                    ret = ac_path_prv();
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once)
                    memcpy(in_points, scratch, in_numpts*sizeof(Point));
                ret = ac_path_prv();
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
        ret = ac_path_prv();

    in_out->clear_property_queue();
    return (ret);
}


bool
gds_in::ac_path_prv()
{
    int pwidth = in_pwidth;
    if (in_transform) {
        TPath(in_numpts, in_points);
        pwidth = mmRnd(in_pwidth * TGetMagn());
    }
    Wire w(pwidth, (WireStyle)in_ptype, in_numpts, in_points);

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
                for (CDll *ll = in_layers; ; ) {
                    ret = in_out->write_poly(&p->po);
                    if (!ret)
                        break;
                    if (!ll)
                        break;
                    ll = ll->next;
                    if (!ll)
                        break;
                    Layer layer(ll->ldesc->name(), in_curlayer,
                        in_curdtype);
                    ret = in_out->queue_layer(&layer);
                    if (!ret)
                        break;
                }
                if (!ret)
                    break;
            }
            PolyList::destroy(pl);
        }
        if (need_out) {
            for (CDll *ll = in_layers; ; ) {
                ret = in_out->write_wire(&w);
                if (!ret)
                    break;
                if (!ll)
                    break;
                ll = ll->next;
                if (!ll)
                    break;
                Layer layer(ll->ldesc->name(), in_curlayer, in_curdtype);
                ret = in_out->queue_layer(&layer);
                if (!ret)
                    break;
            }
        }
        TPop();
    }
    delete [] wp.points;
    return (ret);
}


bool
gds_in::ac_sref()
{
    return (ac_instance(false));
}


bool
gds_in::ac_aref()
{
    return (ac_instance(true));
}


bool
gds_in::ac_instance(bool ary)
{
    if (!read_element())
        return (false);

    if (in_chd_state.gen()) {
        delete [] in_string;
        in_string = 0;
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
        if (dx == 0 && at.nx > 1)
            at.nx = 1;
        if (dy == 0 && at.ny > 1)
            at.ny = 1;

        Instance inst;
        inst.magn = at.magn;
        inst.name = Tstring(cellname);
        inst.nx = at.nx;
        inst.ny = at.ny;
        inst.dx = dx;
        inst.dy = dy;
        inst.origin.set(x, y);
        inst.reflection = at.refly;
        inst.set_angle(at.ax, at.ay);

        if (!ac_instance_backend(&inst, cp, (fst == ts_set)))
            return (false);
        in_out->clear_property_queue();
        return (true);
    }

    if (info())
        info()->add_inst(ary);

    bool ret = true;
    if (!in_ignore_inst) {
        const char *cellname = alias(in_string);

        int ax, ay;
        char *emsg = FIO()->GdsParamSet(&in_angle, &in_magn, &ax, &ay);
        if (emsg) {
            warning(emsg, cellname, in_points->x, in_points->y);
            delete [] emsg;
        }

        CDap ap;
        if (ary) {
            if (!in_nx)
                in_nx = 1;
            if (!in_ny)
                in_ny = 1;
            // The pts are three coordinates of transformed points of the
            // box:
            //   in_points[0] (PREF)  0, 0
            //   in_points[1] (PCOL)  dx, 0
            //   in_points[2] (PROW)  0, dy
            // Apply the inverse transform to get dx, dy.
            //
            Point tp[3];
            tp[0] = in_points[0];
            tp[1] = in_points[1];
            tp[2] = in_points[2];
            TPush();
            // defer de-magnification until after difference
            TApply(tp[0].x, tp[0].y, ax, ay, 1.0, in_reflection);
            TInverse();
            TLoadInverse();
            TPoint(&tp[0].x, &tp[0].y);
            TPoint(&tp[1].x, &tp[1].y);
            TPoint(&tp[2].x, &tp[2].y);
            TPop();
            ap.nx = in_nx;
            ap.ny = in_ny;
            ap.dx = mmRnd((tp[1].x - tp[0].x)/(in_nx*in_magn));
            ap.dy = mmRnd((tp[2].y - tp[0].y)/(in_ny*in_magn));
            if (ap.dx == 0 && ap.nx > 1)
                ap.nx = 1;
            if (ap.dy == 0 && ap.ny > 1)
                ap.ny = 1;
        }

        Instance inst;
        inst.magn = in_magn;
        inst.angle = in_angle;
        inst.name = cellname;
        inst.nx = ap.nx;
        inst.ny = ap.ny;
        inst.dx = ap.dx;
        inst.dy = ap.dy;
        inst.origin.set(in_points[0]);
        inst.reflection = in_reflection;
        inst.ax = ax;
        inst.ay = ay;

        ret = ac_instance_backend(&inst, 0, false);
    }
    in_out->clear_property_queue();
    delete [] in_string;
    in_string = 0;
    return (ret);
}


bool
gds_in::ac_instance_backend(Instance *inst, symref_t *p, bool no_area_test)
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


// Translate a text label.
//
bool
gds_in::ac_text()
{
    if (!read_element())
        return (false);
    return (ac_text_core());
}


// Translate a text label, the main work is done here.
//
bool
gds_in::ac_text_core()
{
    if (in_ignore_text) {
        delete [] in_string;
        in_string = 0;
        in_out->clear_property_queue();
        return (true);
    }

    cl_type cvt = check_layer_cvt();
    if (cvt == cl_error) {
        delete [] in_string;
        in_string = 0;
        in_out->clear_property_queue();
        return (false);
    }
    if (cvt == cl_skip) {
        delete [] in_string;
        in_string = 0;
        in_out->clear_property_queue();
        return (true);
    }
    if (info())
        info()->add_text();

    double oldang = in_angle;
    int ax, ay;
    char *emsg = FIO()->GdsParamSet(&in_angle, &in_magn, &ax, &ay);
    if (emsg) {
        warning(emsg, in_points->x, in_points->y);
        delete [] emsg;
    }
    else
        oldang = in_angle;

    int xform = xform_rot_bits(ax, ay);
    if (in_reflection)
        xform |= TXTF_MY;  // mirror about x axis, before rotation

    // horizontal justification
    int hj = 0;
    if (in_presentation & 0x1) {
        xform |= TXTF_HJC;
        hj = 1;
    }
    else if (in_presentation & 0x2) {
        xform |= TXTF_HJR;
        hj = 2;
    }

    // vertical justification
    int vj = 2;
    if (in_presentation & 0x4) {
        xform |= TXTF_VJC;
        vj = 1;
    }
    else if (!(in_presentation & 0x8)) {
        xform |= TXTF_VJT;
        vj = 0;
    }
    int ft = (in_presentation & 0x30) >> 4; // font
    TXTF_SET_FONT(xform, ft);

    int wid, hei;
    if (in_text_width > 0 && in_text_height > 0) {
        wid = in_text_width;
        hei = in_text_height;
        in_text_width = 0;
        in_text_height = 0;
        if (in_needs_mult) {
            wid = scale(wid);
            hei = scale(hei);
        }
        xform |= (in_text_flags &
            (TXTF_SHOW | TXTF_HIDE | TXTF_TLEV | TXTF_LIML));
        in_text_flags = 0;
    }
    else {
        // This is the displayed string, not necessarily the same as the
        // label text
        hyList *hpl = new hyList(in_sdesc, in_string, HYcvAscii);
        char *string = hyList::string(hpl, HYcvPlain, false);
        double tw, th;
        CD()->DefaultLabelSize(string, in_mode, &tw, &th);
        delete [] string;
        hyList::destroy(hpl);
        if (in_mode == Physical) {
            wid = INTERNAL_UNITS(tw*in_ext_phys_scale*in_magn);
            hei = INTERNAL_UNITS(th*in_ext_phys_scale*in_magn);
        }
        else {
            wid = ELEC_INTERNAL_UNITS(tw*in_magn);
            hei = ELEC_INTERNAL_UNITS(th*in_magn);
        }
    }

    if (in_mode == Physical && (oldang != 0.0 || in_magn != 1.0 ||
            in_pwidth || in_ptype)) {
        // String to store stream stuff we don't use
        char buf[256];
        *buf = 0;
        char *s = buf;
        if (oldang != 0.0) {
            s = lstring::stpcpy(s, " ANGLE ");
            s = mmDtoA(s, oldang, 6);
        }
        if (in_magn != 1.0) {
            s = lstring::stpcpy(s, " MAG ");
            s = mmDtoA(s, in_magn, 6);
        }
        if (in_pwidth) {
            s = lstring::stpcpy(s, " WIDTH ");
            s = mmItoA(s, in_pwidth);
        }
        if (in_ptype) {
            s = lstring::stpcpy(s, " PTYPE ");
            s = mmItoA(s, in_ptype);
        }
        if (*buf)
            in_out->queue_property(II_TEXT + GDSII_PROPERTY_BASE, buf);
    }

    Text text;
    text.text = in_string;
    text.x = in_points->x;
    text.y = in_points->y;
    text.width = wid;
    text.height = hei;
    text.xform = xform;
    text.magn = in_magn*in_scale;
    text.angle = in_angle;
    text.font = ft;
    text.hj = hj;
    text.vj = vj;
    text.pwidth = scale(in_pwidth);
    text.ptype = in_ptype;
    text.reflection = in_reflection;
    text.gds_valid = true;

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

    in_out->clear_property_queue();
    delete [] in_string;
    in_string = 0;
    return (ret);
}


bool
gds_in::ac_text_prv(const Text *ctext)
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

    for (CDll *ll = in_layers; ; ) {
        if (!in_out->write_text(&text))
            return (false);
        if (!ll)
            break;
        ll = ll->next;
        if (!ll)
            break;
        Layer layer(ll->ldesc->name(), in_curlayer, in_curdtype);
        if (!in_out->queue_layer(&layer))
            return (false);
    }
    return (true);
}


bool
gds_in::ac_snapnode()
{
    if (!read_element())
        return (false);
    in_out->clear_property_queue();
    return (true);
}


bool
gds_in::ac_propvalue()
{
    if ((in_attrib == XICP_GDS_LABEL_SIZE ||
            in_attrib == OLD_GDS_LABEL_SIZE_PROP) && in_elemrec) {
        if (read_text_property())
            return (true);
    }
    if (in_ignore_prop)
        return (true);
    CDp *px = new CDp(in_cbuf, in_attrib);
    px->scale(1.0, in_phys_scale, in_mode);
    bool ret = in_out->queue_property(px->value(), px->string());
    delete px;
    in_attrib = -1;
    return (ret);
}


bool
gds_in::ac_box()
{
    return (ac_boundary());
}

