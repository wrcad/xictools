
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
#include "fio_oasis.h"
#include "fio_oas_reps.h"
#include "fio_cgd.h"
#include "fio_cgd_lmux.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "cd_chkintr.h"
#include "geo_zlist.h"
#include "filestat.h"
#include <ctype.h>


// Convert the hierarchies under the named symbols to OASIS format
// according to the parameters.  The scale is used for the physical
// data.
//
// Return values:
//  OIok        normal return
//  OIerror     unspecified error (message in Errs)
//  OIabort     user interrupt
//
OItype
cFIO::ToOASIS(const stringlist *namelist, const FIOcvtPrms *prms)
{
    if (!namelist) {
        Errs()->add_error("Error in ToOASIS: null symbol list.");
        return (OIerror);
    }
    if (!prms) {
        Errs()->add_error("Error in ToOASIS: null params pointer.");
        return (OIerror);
    }

    const char *oas_fname = prms->destination();
    if (!oas_fname || !*oas_fname) {
        Errs()->add_error("Error in ToOASIS: null file name.");
        return (OIerror);
    }

    if (!namelist->next && namelist->string &&
            !strcmp(namelist->string, FIO_CUR_SYMTAB)) {
        // Secret code to dump the entire current symbol table.

        oas_out *oas = new oas_out(0);
        GCobject<oas_out*> gc_oas(oas);
        if (!oas->set_destination(oas_fname))
            return (OIerror);
        oas->enable_cache(true);
        oas->assign_alias(NewWritingAlias(prms->alias_mask(), false));
        oas->read_alias(oas_fname);

        bool ok = oas->write_all(Physical, prms->scale());
        if (ok && !fioStripForExport)
            ok = oas->write_all(Electrical, prms->scale());
        return (ok ? OIok : oas->was_interrupted() ? OIaborted : OIerror);
    }

    for (const stringlist *sl = namelist; sl; sl = sl->next) {
        if (!CDcdb()->findSymbol(sl->string)) {
            Errs()->add_error("Error in ToOASIS: %s not in symbol table.",
                sl->string ? sl->string : "(null)");
            return (OIerror);
        }
        if (prms->flatten())
            break;
    }

    oas_out *oas = new oas_out(0);
    GCobject<oas_out*> gc_oas(oas);
    if (!oas->set_destination(oas_fname))
        return (OIerror);
    oas->enable_cache(true);
    oas->assign_alias(NewWritingAlias(prms->alias_mask(), false));
    oas->read_alias(oas_fname);

    const BBox *AOI = prms->use_window() ? prms->window() : 0;
    bool ok;
    if (prms->flatten())
        // Just one name allowed, physical mode only.
        ok = oas->write_flat(namelist->string, prms->scale(), AOI,
            prms->clip());
    else {
        ok = oas->write(namelist, Physical, prms->scale());
        if (ok && !fioStripForExport) {
            oas->enable_cache(false);
            ok = oas->write(namelist, Electrical, prms->scale());
        }
    }
    return (ok ? OIok : oas->was_interrupted() ? OIaborted : OIerror);
}


// Write the list of zoids to an OASIS file, in a dummy cell.  The lname
// is the name of an existing layer with a stream output mapping, or a
// hex layer/datatype string.
//
bool
cFIO::ZlistToFile(Zlist *zlist, const char *lname, const char *fname)
{
    if (!lname)
        return (false);
    ::Layer layer;
    layer.name = lname;

    oas_out *oas = new oas_out(0);
    if (!oas->set_destination(fname)) {
        delete oas;
        return (false);
    }
    oas->enable_cache(false);

    bool ret = oas->write_header(0);
    if (ret) {
        time_t tloc = time(0);
        tm *date = gmtime(&tloc);
        ret = oas->write_struct("zoidlist", date, date);
    }
    if (ret)
        ret = oas->queue_layer(&layer);
    if (ret) {
        for (Zlist *z = zlist; z; z = z->next) {
            if (!oas->write_trapezoid(&z->Z, false)) {
                ret = false;
                break;
            }
        }
    }
    if (ret)
        ret = oas->write_end_struct();
    if (ret)
        ret = oas->write_endlib(0);
    delete oas;
    return (ret);
}
// End of cCD functions


#define NOT_MODAL(x, y) !out_modal->x##_set || \
 (unsigned int)(y) != (unsigned int)out_modal->x
#define MODAL_ASSIGN(x, y) out_modal->x = y, out_modal->x##_set = true


// If the arg is true, we are writing to an in-core database rather
// than a file.  The in_filename is the database tag name.  In this
// mode, the OasWrite... variables are ignored - maximum compression
// is mandated.  The created database is saved in the front-end
// in the destructor.
//
oas_out::oas_out(cCGD *cgd)
{
    bool use_cgd = (cgd != 0);

    out_filetype = Foas;
    out_fp = 0;
    out_tmp_fp = 0;
    out_tmp_fname = 0;
    out_layer_cache = 0;

    out_cellname_tab = 0;
    out_textstring_tab = 0;
    out_propname_tab = 0;
    out_propstring_tab = 0;
    out_layername_tab = 0;
    out_xname_tab = 0;
    out_string_pool = 0;

    out_cellname_ix = 0;
    out_textstring_ix = 0;
    out_propname_ix = 0;
    out_propstring_ix = 0;
    out_layername_ix = 0;
    out_xname_ix = 0;

    out_cellname_off = 0;
    out_textstring_off = 0;
    out_propname_off = 0;
    out_propstring_off = 0;
    out_layername_off = 0;
    out_xname_off = 0;

    out_comp_start = 0;
    out_offset_table = 0;
    out_start_offset = 0;

    out_cgd = cgd;
    out_lmux = 0;
    out_modal = &out_default_modal;
    out_zfile = 0;
    out_cache = 0;
    out_compr_buf = 0;
    if (use_cgd)
        out_rep_args = lstring::copy("");
    else
        out_rep_args = lstring::copy(FIO()->OasWriteRep());
    out_undef_count = 0;

    if (use_cgd)
        out_prp_mask = OAS_PRPMSK_ALL;
    else {
        out_prp_mask = FIO()->OasWritePrptyMask();
        if (!FIO()->IsAllowPrptyStrip())
            out_prp_mask &= ~OAS_PRPMSK_ALL;
    }
    out_offset_flag = false;
    out_use_repetition = false;
    if (use_cgd) {
        out_use_trapezoids = true;
        out_boxes_for_wires = true;
        out_rnd_wire_to_poly = true;
        out_faster_sort = false;
        out_no_gcd_check = false;
    }
    else {
        out_use_trapezoids = !FIO()->IsOasWriteNoTrapezoids();
        out_boxes_for_wires = FIO()->IsOasWriteWireToBox();
        out_rnd_wire_to_poly = FIO()->IsOasWriteRndWireToPoly();
        out_faster_sort = FIO()->IsOasWriteUseFastSort();
        out_no_gcd_check = FIO()->IsOasWriteNoGCDcheck();
    }

    if (use_cgd)
        out_use_compression = OAScompSmart;  // normal compression
    else
        out_use_compression = FIO()->OasWriteCompressed();
    out_compressing = false;
    out_use_tables = use_cgd ? true : FIO()->IsOasWriteNameTab();

    if (use_cgd)
        out_validation_type = 0;
    else
        out_validation_type = FIO()->OasWriteChecksum();
}


oas_out::~oas_out()
{
    if (out_cgd) {
        if (out_filename)
            CDcgd()->cgdStore(lstring::strip_path(out_filename), out_cgd);
        else
            delete out_cgd;
    }

    if (out_zfile)
        delete out_zfile;
    if (out_fp)
        fclose(out_fp);
    if (out_tmp_fp)
        fclose(out_tmp_fp);
    if (out_tmp_fname) {
        unlink(out_tmp_fname);
        delete [] out_tmp_fname;
    }
    if (out_layer_cache) {
        out_layer_cache->clear();
        delete out_layer_cache;
    }
    delete out_lmux;
    delete out_cache;
    delete out_cellname_tab;
    delete out_textstring_tab;
    delete out_propname_tab;
    delete out_propstring_tab;
    delete out_layername_tab;
    delete out_xname_tab;
    delete out_string_pool;
    delete [] out_compr_buf;
    delete [] out_rep_args;
}


bool
oas_out::check_for_interrupt()
{
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
    return (true);
}


bool
oas_out::write_header(const CDs*)
{
    if (!write_library(0, 0.0, 0.0, 0, 0, 0))
        return (false);
    return (true);
}


// This is only called when writing from memory, and only after
// queue_layer has been called.
//
// This will not be called if the present layer has no resolvable
// output mapping.
//
// If the layer has a strm_odata, use that, which handles multiple
// mapping.  Otherwise, assume that out_layer.layer/out_layer.datatype
// have previously been set be some means.
//
// Write an object, properties are already queued.
//
bool
oas_out::write_object(const CDo *odesc, cvLchk *lchk)
{
    if (!odesc)
        return (true);
    CDl *ldesc = odesc->ldesc();

    if (lchk && *lchk == cvLneedCheck) {
        bool brk = false;
        if (!check_set_layer(ldesc, &brk))
            return (false);
        if (brk) {
            *lchk = cvLnogo;
            return (true);
        }
        *lchk = cvLok;
    }

    if (!ldesc->strmOut())
        return (write_object_prv(odesc));
    unsigned int ndrcdt = ldesc->getStrmNoDrcDatatype(odesc);
    unsigned int lastl = -1;
    for (strm_odata *so = ldesc->strmOut(); so; so = so->next()) {
        if (so->layer() != lastl) {
            out_layer.layer = so->layer();
            lastl = so->layer();
            if (ndrcdt < GDS_MAX_DTYPES) {
                out_layer.datatype = ndrcdt;
                if (!write_object_prv(odesc))
                    return (false);
                if (!FIO()->IsMultiLayerMapOk())
                    break;
            }
        }
        else if (ndrcdt < GDS_MAX_DTYPES)
            continue;

        out_layer.datatype = so->dtype();
        if (!write_object_prv(odesc))
            return (false);
        if (!FIO()->IsMultiLayerMapOk())
            break;
    }
    return (true);
}


bool
oas_out::set_destination(const char *destination)
{
    if (!out_cgd) {
        out_fp = large_fopen(destination, "wb");
        if (!out_fp) {
            Errs()->sys_error("open");
            Errs()->add_error("Unable to open %s.", destination);
            return (false);
        }
    }
    out_filename = lstring::copy(destination);
    return (true);
}


// When switching to a new output channel, we have to save/recall the
// modality state.  This is used in cCHD::readSplit.
//
bool
oas_out::set_destination(FILE *fp, void **new_cx, void **old_cx)
{
    if (old_cx) {
        if (!*old_cx)
            *old_cx = new char[sizeof(oas_modal)];
        memcpy(*old_cx, out_modal, sizeof(oas_modal));
    }
    if (new_cx) {
        if (!*new_cx)
            out_modal->reset();
        else
            memcpy(out_modal, *new_cx, sizeof(oas_modal));
    }
    out_fp = fp;
    return (true);
}


bool
oas_out::queue_property(int val, const char *string)
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
oas_out::open_library(DisplayMode mode, double sc)
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

    if (!write_library(0, 0.0, 0.0, 0, 0, 0))
        return (false);

    return (true);
}


// Leave this many bytes for the offset table in the START record.  We
// need to leave room for six byte flags plus six offsets.  Extra bytes
// become PAD records.
#define OFFSET_TABLE_SIZE 48

bool
oas_out::write_library(int, double, double, tm*, tm*, const char*)
{
    enable_cache((out_mode == Physical));
    if (out_cgd)
        return (true);

    // Save START position for checksum computation.  This is of course
    // 0, except for the electrical records extension.
    out_start_offset = large_ftell(out_fp);

    // write file magic header
    const char *s = OAS_MAGIC;
    while (*s) {
        if (!write_char(*s))
            return (false);
        s++;
    }

    // setup use of tables
    if (out_use_tables)
        init_tables(out_mode);

    // write START index
    if (!write_unsigned(1))
        return (false);

    // write version number
    s = "1.0";
    if (!write_a_string(s))
        return (false);

    // write unit
    if (out_mode == Physical) {
        if (!write_real(CDphysResolution))
            return (false);
    }
    else {
        if (!write_real(CDelecResolution))
            return (false);
    }

    if (out_offset_flag) {
        // write offset-flag: 1 for tables in END record
        if (!write_unsigned(1))
            return (false);
    }
    else {
        // write offset-flag: 0 for tables in START record
        if (!write_unsigned(0))
            return (false);
        // If using tables, save offset, and fill space with null bytes.
        out_offset_table = large_ftell(out_fp);
        int space = out_string_pool ? OFFSET_TABLE_SIZE : 12;
        while (space--) {
            if (!write_char(0))
                return (false);
        }
    }

    // Write a property that marks the file as being generated by Xic.
    if (!write_unsigned(28))
        return (false);
    const char *pname = "XIC_SOURCE";
    unsigned char info_byte = 0x4;  // UUUUVCNS
    if (!write_char(info_byte))
        return (false);
    if (!write_n_string(pname))
        return (false);

    return (true);
}


bool
oas_out::write_struct(const char *name, tm*, tm*)
{
    if (out_no_struct) {
        out_in_struct = true;
        return (true);
    }
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
        clear_property_queue();

    name = alias(name);
    out_in_struct = true;
    out_struct_count++;
    out_modal->reset();
    out_cellBB = CDnullBB;
    out_layer.set_null();

    if (!out_cellname_tab) {
        // write CELL index (local name)
        if (!write_unsigned(14))
            return (false);

        // write cell name
        if (!write_n_string(name))
            return (false);
    }
    else {
        unsigned long refnum = (unsigned long)SymTab::get(
            out_cellname_tab, name);
        if (refnum == (unsigned long)ST_NIL) {
            refnum = out_cellname_ix++;
            out_cellname_tab->add(out_string_pool->new_string(name),
                (void*)refnum, false);
        }

        // write CELL index (from table)
        if (!write_unsigned(13))
            return (false);

        // write reference number
        if (!write_unsigned(refnum))
            return (false);
    }
    if (!begin_compression(name))
        return (false);

    // write cell properties
    if (!write_cell_properties())
        return (false);
    return (true);
}


bool
oas_out::write_end_struct(bool)
{
    if (out_in_struct) {
        if (out_no_struct)
            return (true);
        if (!flush_cache())
            return (false);
        out_in_struct = false;
        if (!end_compression())
            return (false);
    }
    return (true);
}


namespace {
    // If str is int he form Lnnn or lnnn where n's are digits, return
    // true and the numeric value of nnn.
    //
    bool
    parse_lnum(const char *str, int *num)
    {
        if (*str == 'L' || *str == 'l') {
            for (const char *s = str+1; s; s++) {
                if (!isdigit(*s))
                    return (false);
            }
            *num = atoi(str+1);
            return (true);
        }
        return (false);
    }
}


bool
oas_out::queue_layer(const Layer *layer, bool *check_mapping)
{
    if (!out_in_struct)
        return (true);

    if (check_mapping) {
        *check_mapping = false;

        // If check_mapping is not null, then we are dumping from
        // memory, and we need to test if the current layer has a
        // gdsii output mapping.  If there is no mapping that can be
        // resolved, true is returned in this pointer.
        //
        // A hash table is used here to speed things up.  The layer
        // names are known to be from the layer table.

        if (!out_layer_cache)
            out_layer_cache = new table_t<tmp_odata>;
        tmp_odata *td = out_layer_cache->find(layer->name);

        if (!td) {
            int l, d;
            CDl *ld;
            CDl *ldesc = CDldb()->findLayer(layer->name, out_mode);
            if (!ldesc) {
                // "can't happen"
                Errs()->add_error(
                    "queue_layer: (internal) %s not in layer table.",
                    ldesc->name());
                *check_mapping = true;
                return (false);
            }
            if (ldesc->strmOut()) {
                td = new tmp_odata;
                td->set_name(ldesc->name());
                td->set_layer(ldesc->strmOut()->layer());
                td->set_dtype(ldesc->strmOut()->dtype());
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
            }
            else if (out_mode == Electrical &&
                    !strcmp(ldesc->name(), "INIT") &&
                    (ld = CDldb()->findLayer("PARM", out_mode)) != 0 &&
                    ld->strmOut()) {
                // foolishness for backward compatibility
                td = new tmp_odata;
                td->set_name(ldesc->name());
                td->set_layer(ld->strmOut()->layer());
                td->set_dtype(ld->strmOut()->dtype());
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
            }
            else if (strmdata::hextrn(ldesc->name(), &l, &d)) {
                // The layer name is a hex number "LLDD" or "LLLLDDDD".
                td = new tmp_odata;
                td->set_name(ldesc->name());
                td->set_layer(l);
                td->set_dtype(d);
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
            }
            else if (ldesc->lppName() &&
                    strmdata::hextrn(ldesc->lppName(), &l, &d)) {
                // The LPP name is a hex number "LLDD" or "LLLLDDDD".
                td = new tmp_odata;
                td->set_name(ldesc->name());
                td->set_layer(l);
                td->set_dtype(d);
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
            }
            else {
                // In electrical mode, if the layer number is reserved
                // in Virtuoso (see tech_cds_in.cc), use this and the
                // purpose num for the mapping.

                if (out_mode == Electrical) {
                    unsigned int oalnum = ldesc->oaLayerNum();
                    unsigned int oapnum = ldesc->oaPurposeNum();
                    // The Virtuoso reserved "drawing" purpose is 252.
                    if (oapnum == (unsigned int)-1)
                        oapnum = 252;
                    if (oalnum >= 200 && oalnum <= 255 && oapnum <= 255) {
                        td = new tmp_odata;
                        td->set_name(ldesc->name());
                        td->set_layer(oalnum);
                        td->set_dtype(oapnum);
                        out_layer_cache->link(td, false);
                        out_layer_cache = out_layer_cache->check_rehash();
                    }
                }
            }
            if (!td) {
                // Uh-oh, no mapping found.
                *check_mapping = true;
                if (!FIO()->IsNoGdsMapOk()) {
                    // Fatal, no output mapping specified.
                    Errs()->add_error(
                        "no GDS output mapping for layer %s.", ldesc->name());
                    return (false);
                }
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "no GDS output mapping for layer %s (ignored).",
                    ldesc->name());
                ldesc->setTmpSkip(true);
                return (true);
            }
        }

        out_layer.layer = td->layer();
        out_layer.datatype = td->dtype();
        return (true);
    }

    // If we get a layer with a name but layer and datatype set to -1,
    // we are translating from a format that does not use these, and
    // we need to map to some reasonable values.  First look in the
    // Xic layer table for a mapping, and if not found define our own
    // mapping.
    //
    int l = layer->layer;
    int d = layer->datatype;

    // Include 0,0 in the test, for backwards compatibility with CGX
    // files.
    if (l <= 0 && d <= 0 && layer->name) {

        if (!out_layer_cache)
            out_layer_cache = new table_t<tmp_odata>;
        tmp_odata *td = out_layer_cache->find(layer->name);

        if (!td) {
            const char *msg = "Mapping layer %s to layer %d, datatype %d.";
            CDl *ld = CDldb()->findLayer(layer->name, out_mode);
            if (ld && ld->strmOut()) {
                td = new tmp_odata;
                td->set_name(layer->name);
                td->set_layer(ld->strmOut()->layer());
                td->set_dtype(ld->strmOut()->dtype());
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
            }
            else if (out_mode == Electrical && !strcmp(layer->name, "INIT") &&
                    (ld = CDldb()->findLayer("PARM", out_mode)) != 0 &&
                    ld->strmOut()) {
                // foolishness for backward compatibility
                td = new tmp_odata;
                td->set_name(layer->name);
                td->set_layer(ld->strmOut()->layer());
                td->set_dtype(ld->strmOut()->dtype());
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
            }
            else if (strmdata::hextrn(layer->name, &l, &d)) {
                // name is hex number LLDD or LLLLDDDD
                td = new tmp_odata;
                td->set_name(layer->name);
                td->set_layer(l);
                td->set_dtype(d);
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
                FIO()->ifPrintCvLog(IFLOG_WARN, msg, layer->name, l, d);
            }
            else if (parse_lnum(layer->name, &l)) {
                // name is Lnnn or lnnn
                td = new tmp_odata;
                td->set_name(layer->name);
                td->set_layer(l);
                td->set_dtype(0);
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
                FIO()->ifPrintCvLog(IFLOG_WARN, msg, layer->name, l, d);
            }
            else {
                // Make up our own layer/datatype for unresolvable layer
                // name.  Note that there is no testing for existing use
                // of this layer/datatype.

                l = FIO()->UnknownGdsLayerBase() + out_undef_count++;
                d = FIO()->UnknownGdsDatatype();

                td = new tmp_odata;
                td->set_name(layer->name);
                td->set_layer(l);
                td->set_dtype(d);
                out_layer_cache->link(td, false);
                out_layer_cache = out_layer_cache->check_rehash();
                FIO()->ifPrintCvLog(IFLOG_WARN, msg, layer->name, l, d);
            }
        }
        l = td->layer();
        d = td->dtype();
    }
    if (l < 0 || d < 0) {
        Errs()->add_error(
            "negative layer or datatype encountered, aborting.");
        return (false);
    }

    if (out_lmux)
        out_modal = out_lmux->set_layer(l, d);

    out_layer.layer = l;
    out_layer.datatype = d;
    return (true);
}


bool
oas_out::write_box(const BBox *box)
{
    if (!check_for_interrupt())
        return (false);
    if (out_in_struct) {
        if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
            clear_property_queue();

        if (out_cache && out_cache->caching_boxes()) {
            bool ret = out_cache->cache_box(box, out_layer.layer,
                out_layer.datatype, out_prpty);
            out_prpty = 0;
            if (ret)
                ret = out_cache->check_flush_box();
            return (ret);
        }

        // 20 rectangle-info-byte [layer-number] [datatype-number]
        // [width] [height] [x] [y] [repetition]

        // write RECTANGLE index
        if (!write_unsigned(20))
            return (false);

        unsigned char info_byte = 0x0;  // SWHXYRDL
        unsigned int width = scale(box->width());
        unsigned int height = scale(box->height());
        int x = scale(box->left);
        int y = scale(box->bottom);
        if (width == height)
            info_byte |= 0x80;
        if (NOT_MODAL(geometry_w, width))
            info_byte |= 0x40;
        if (width != height) {
            if (NOT_MODAL(geometry_h, height))
                info_byte |= 0x20;
        }
        if (NOT_MODAL(geometry_x, x))
            info_byte |= 0x10;
        if (NOT_MODAL(geometry_y, y))
            info_byte |= 0x8;
        if (out_use_repetition)
            info_byte |= 0x4;
        if (NOT_MODAL(datatype, out_layer.datatype))
            info_byte |= 0x2;
        if (NOT_MODAL(layer, out_layer.layer))
            info_byte |= 0x1;
        if (!write_char(info_byte))
            return (false);

        if (NOT_MODAL(layer, out_layer.layer)) {
            if (!write_unsigned(out_layer.layer))
                return (false);
            MODAL_ASSIGN(layer, out_layer.layer);
        }
        if (NOT_MODAL(datatype, out_layer.datatype)) {
            if (!write_unsigned(out_layer.datatype))
                return (false);
            MODAL_ASSIGN(datatype, out_layer.datatype);
        }
        if (NOT_MODAL(geometry_w, width)) {
            if (!write_unsigned(width))
                return (false);
            MODAL_ASSIGN(geometry_w, width);
        }
        if (NOT_MODAL(geometry_h, height)) {
            if (width != height) {
                if (!write_unsigned(height))
                    return (false);
            }
            MODAL_ASSIGN(geometry_h, height);
        }
        if (NOT_MODAL(geometry_x, x)) {
            if (!write_signed(x))
                return (false);
            MODAL_ASSIGN(geometry_x, x);
        }
        if (NOT_MODAL(geometry_y, y)) {
            if (!write_signed(y))
                return (false);
            MODAL_ASSIGN(geometry_y, y);
        }

        // write repetition
        if (out_use_repetition && !write_repetition())
            return (false);

        // write properties
        if (!write_elem_properties())
            return (false);
    }
    return (true);
}


namespace {
    // If po can be represented as a trapezoid, fill in z and return true.
    // If the trapezoid is vertical, vert is set, and the meaning of the
    // zoid elements becomes: yl/yu -> xl/xr, xll/xlr -> ylb/ylt,
    // xul/xur -> yrb/yrt.
    //
    bool
    is_trap(const Poly *po, Zoid *z, bool *vert)
    {
        Point *pts = po->points;
        if (po->numpts == 4) {
            for (int i = 0; i < 3; i++) {
                int n = i+2;
                if (n == 4)
                    n = 1;

                if (pts[i].y == pts[i+1].y) {
                    if (pts[n].y > pts[i].y) {
                        *vert = false;
                        *z = Zoid(
                            mmMin(pts[i].x, pts[i+1].x),
                            mmMax(pts[i].x, pts[i+1].x),
                            pts[i].y,
                            pts[n].x,
                            pts[n].x,
                            pts[n].y);
                        return (true);
                    }
                    else {
                        *vert = false;
                        *z = Zoid(
                            pts[n].x,
                            pts[n].x,
                            pts[n].y,
                            mmMin(pts[i].x, pts[i+1].x),
                            mmMax(pts[i].x, pts[i+1].x),
                            pts[i].y);
                        return (true);
                    }
                }
                if (pts[i].x == pts[i+1].x) {
                    if (pts[n].x > pts[i].x) {
                        *vert = true;
                        *z = Zoid(
                            mmMin(pts[i].y, pts[i+1].y),
                            mmMax(pts[i].y, pts[i+1].y),
                            pts[i].x,
                            pts[n].y,
                            pts[n].y,
                            pts[n].x);
                        return (true);
                    }
                    else {
                        *vert = true;
                        *z = Zoid(
                            pts[n].y,
                            pts[n].y,
                            pts[n].x,
                            mmMin(pts[i].y, pts[i+1].y),
                            mmMax(pts[i].y, pts[i+1].y),
                            pts[i].x);
                        return (true);
                    }
                }
            }
        }
        else if (po->numpts == 5) {
            for (int pass = 0; pass < 2; pass++) {
                if (pts[0].y == pts[1].y && pts[2].y == pts[3].y) {
                    if ((pts[0].x < pts[1].x && pts[3].x < pts[2].x) ||
                            (pts[0].x > pts[1].x && pts[3].x > pts[2].x)) {
                        int nl = pts[0].y < pts[2].y ? 0 : 2;
                        int nu = pts[0].y < pts[2].y ? 2 : 0;
                        *vert = false;
                        *z = Zoid(
                            mmMin(pts[nl].x, pts[nl+1].x),
                            mmMax(pts[nl].x, pts[nl+1].x),
                            pts[nl].y,
                            mmMin(pts[nu].x, pts[nu+1].x),
                            mmMax(pts[nu].x, pts[nu+1].x),
                            pts[nu].y);
                        return (true);
                    }
                }
                if (pts[0].x == pts[1].x && pts[2].x == pts[3].x) {
                    if ((pts[0].y < pts[1].y && pts[3].y < pts[2].y) ||
                            (pts[0].y > pts[1].y && pts[3].y > pts[2].y)) {
                        int nl = pts[0].x < pts[2].x ? 0 : 2;
                        int nu = pts[0].x < pts[2].x ? 2 : 0;
                        *vert = true;
                        *z = Zoid(
                            mmMin(pts[nl].y, pts[nl+1].y),
                            mmMax(pts[nl].y, pts[nl+1].y),
                            pts[nl].x,
                            mmMin(pts[nu].y, pts[nu+1].y),
                            mmMax(pts[nu].y, pts[nu+1].y),
                            pts[nu].x);
                        return (true);
                    }
                }
                pts++;
            }
        }
        return (false);
    }
}


bool
oas_out::write_poly(const Poly *poly)
{
    if (!check_for_interrupt())
        return (false);

    if (out_in_struct) {
        if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
            clear_property_queue();

        if (out_cache && out_cache->caching_polys()) {
            bool ret = out_cache->cache_poly(poly, out_layer.layer,
                out_layer.datatype, out_prpty);
            out_prpty = 0;
            if (ret)
                ret = out_cache->check_flush_poly();
            return (ret);
        }

        if (out_use_trapezoids) {
            // Check if we can write a (more compact) trapezoid instead.
            Zoid Z;
            bool vert;
            if (is_trap(poly, &Z, &vert)) {
                if (!write_trapezoid(&Z, vert))
                    return (false);
                return (true);
            }
        }

        // 21 polygon-info-byte [layer-number] [datatype-number] [point-list]
        // [x] [y] [repetition]

        // write POLYGON index
        if (!write_unsigned(21))
            return (false);

        unsigned char info_byte = 0x20;  // 00PXYRDL
        int x = scale(poly->points[0].x);
        int y = scale(poly->points[0].y);
        if (NOT_MODAL(geometry_x, x))
            info_byte |= 0x10;
        if (NOT_MODAL(geometry_y, y))
            info_byte |= 0x8;
        if (out_use_repetition)
            info_byte |= 0x4;
        if (NOT_MODAL(datatype, out_layer.datatype))
            info_byte |= 0x2;
        if (NOT_MODAL(layer, out_layer.layer))
            info_byte |= 0x1;
        if (!write_char(info_byte))
            return (false);

        if (NOT_MODAL(layer, out_layer.layer)) {
            if (!write_unsigned(out_layer.layer))
                return (false);
            MODAL_ASSIGN(layer, out_layer.layer);
        }
        if (NOT_MODAL(datatype, out_layer.datatype)) {
            if (!write_unsigned(out_layer.datatype))
                return (false);
            MODAL_ASSIGN(datatype, out_layer.datatype);
        }

        // write point list
        if (!write_point_list(poly->points, poly->numpts, true))
            return (false);

        if (NOT_MODAL(geometry_x, x)) {
            if (!write_signed(x))
                return (false);
            MODAL_ASSIGN(geometry_x, x);
        }
        if (NOT_MODAL(geometry_y, y)) {
            if (!write_signed(y))
                return (false);
            MODAL_ASSIGN(geometry_y, y);
        }

        // write repetition
        if (out_use_repetition && !write_repetition())
            return (false);

        // write properties
        if (!write_elem_properties())
            return (false);
    }
    return (true);
}


bool
oas_out::write_wire(const Wire *wire)
{
    if (!check_for_interrupt())
        return (false);

    if (out_in_struct) {

        if (out_rnd_wire_to_poly && wire->wire_style() == CDWIRE_ROUND) {
            Poly po;
            if (!wire->toPoly(&po.points, &po.numpts)) {
                Errs()->add_error("write_wire: toPoly failed, bad wire.");
                return (false);
            }
            bool ret = write_poly(&po);
            delete [] po.points;
            return (ret);
        }

        if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
            clear_property_queue();

        if (out_boxes_for_wires) {
            // Check if we can write a (more compact) rectangle instead.
            if (wire->numpts == 2 && wire->wire_style() != CDWIRE_ROUND &&
                    (wire->points[0].x == wire->points[1].x ||
                    wire->points[0].y == wire->points[1].y)) {

                if (wire->points[1].x == wire->points[0].x) {
                    int hw = wire->wire_width()/2;
                    int xt = wire->wire_style() == CDWIRE_EXTEND ? hw : 0;
                    BBox BB(wire->points[0].x - hw,
                        mmMin(wire->points[0].y, wire->points[1].y) - xt,
                        wire->points[0].x + hw,
                        mmMax(wire->points[0].y, wire->points[1].y) + xt);
                    if (!write_box(&BB))
                        return (false);
                }
                else {
                    int hw = wire->wire_width()/2;
                    int xt = wire->wire_style() == CDWIRE_EXTEND ? hw : 0;
                    BBox BB(mmMin(wire->points[0].x, wire->points[1].x) - xt,
                        wire->points[0].y - hw,
                        mmMax(wire->points[0].x, wire->points[1].x) + xt,
                        wire->points[0].y + hw);
                    if (!write_box(&BB))
                        return (false);
                }
                return (true);
            }
        }

        if (out_cache && out_cache->caching_wires()) {
            bool ret = out_cache->cache_wire(wire, out_layer.layer,
                out_layer.datatype, out_prpty);
            out_prpty = 0;
            if (ret)
                ret = out_cache->check_flush_wire();
            return (ret);
        }

        // 22 path-info-byte [layer-number] [datatype-number] [half-width]
        // [extension-scheme [start-exntension] [end-extension]]
        // [point-list] [x] [y] [repetition]

        // write PATH index
        if (!write_unsigned(22))
            return (false);

        unsigned char info_byte = 0x20;  // EWPXYRDL
        int x = scale(wire->points[0].x);
        int y = scale(wire->points[0].y);
        unsigned int hw = scale(wire->wire_width()/2);
        int etyp = wire->wire_style() == CDWIRE_FLUSH ? 0x1 : 0x2;
        int extn = wire->wire_style() == CDWIRE_FLUSH ? 0 : hw;
        if (NOT_MODAL(path_start_extension, extn) ||
                NOT_MODAL(path_end_extension, extn))
            info_byte |= 0x80;
        if (NOT_MODAL(path_half_width, hw))
            info_byte |= 0x40;
        if (NOT_MODAL(geometry_x, x))
            info_byte |= 0x10;
        if (NOT_MODAL(geometry_y, y))
            info_byte |= 0x8;
        if (out_use_repetition)
            info_byte |= 0x4;
        if (NOT_MODAL(datatype, out_layer.datatype))
            info_byte |= 0x2;
        if (NOT_MODAL(layer, out_layer.layer))
            info_byte |= 0x1;
        if (!write_char(info_byte))
            return (false);

        if (NOT_MODAL(layer, out_layer.layer)) {
            if (!write_unsigned(out_layer.layer))
                return (false);
            MODAL_ASSIGN(layer, out_layer.layer);
        }
        if (NOT_MODAL(datatype, out_layer.datatype)) {
            if (!write_unsigned(out_layer.datatype))
                return (false);
            MODAL_ASSIGN(datatype, out_layer.datatype);
        }
        if (NOT_MODAL(path_half_width, hw)) {
            if (!write_unsigned(hw))
                return (false);
            MODAL_ASSIGN(path_half_width, hw);
        }

        if (NOT_MODAL(path_start_extension, extn) ||
                NOT_MODAL(path_end_extension, extn)) {
            if (!write_unsigned((etyp << 2) | etyp))
                return (false);
            MODAL_ASSIGN(path_start_extension, extn);
            MODAL_ASSIGN(path_end_extension, extn);
        }

        // write point list
        if (!write_point_list(wire->points, wire->numpts, false))
            return (false);

        if (NOT_MODAL(geometry_x, x)) {
            if (!write_signed(x))
                return (false);
            MODAL_ASSIGN(geometry_x, x);
        }
        if (NOT_MODAL(geometry_y, y)) {
            if (!write_signed(y))
                return (false);
            MODAL_ASSIGN(geometry_y, y);
        }

        // write repetition
        if (out_use_repetition && !write_repetition())
            return (false);

        // if necessarey, record PATHTYPE=1
        if (wire->wire_style() == CDWIRE_ROUND) {
            if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
                ;
            else if (!write_rounded_end_property())
                return (false);
        }

        // write properties
        if (!write_elem_properties())
            return (false);
    }
    return (true);
}


bool
oas_out::write_text(const Text *text)
{
    if (!check_for_interrupt())
        return (false);
    if (out_cgd) {
        clear_property_queue();
        return (true);
    }
    if (!out_in_struct)
        return (true);

    if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
        clear_property_queue();

    if (out_cache && out_cache->caching_labels()) {
        bool ret = out_cache->cache_label(text, out_layer.layer,
            out_layer.datatype, out_prpty);
        out_prpty = 0;
        if (ret)
            ret = out_cache->check_flush_label();
        return (ret);
    }

    // 19 text-info-byte [reference-number | text-string]
    // [textlayer-number] [texttype-number] [x] [y] [repetition]

    if (!text->text || !*text->text)
        return (true);

    // write TEXT index
    if (!write_unsigned(19))
        return (false);

    // info byte, use CXY, maybe NDL
    unsigned char info_byte = 0;  // 0CNXYRDL
    int x = scale(text->x);
    int y = scale(text->y);
    char *string = lstring::copy(text->text);
    if (out_mode == Electrical && out_needs_mult)
        string = hyList::hy_scale(string, out_scale);

    if (!out_modal->text_string_set ||
            strcmp(string, out_modal->text_string)) {
        info_byte |= 0x40;
        if (out_textstring_tab)
            info_byte |= 0x20;
    }
    if (NOT_MODAL(text_x, x))
        info_byte |= 0x10;
    if (NOT_MODAL(text_y, y))
        info_byte |= 0x8;
    if (out_use_repetition)
        info_byte |= 0x4;
    if (NOT_MODAL(texttype, out_layer.datatype))
        info_byte |= 0x2;
    if (NOT_MODAL(textlayer, out_layer.layer))
        info_byte |= 0x1;
    if (!write_char(info_byte)) {
        delete [] string;
        return (false);
    }

    if (!out_modal->text_string_set ||
            strcmp(string, out_modal->text_string)) {
        if (!out_textstring_tab) {
            if (!write_a_string(string))
                return (false);
        }
        else {
            unsigned long refnum = (unsigned long)SymTab::get(
                out_textstring_tab, string);
            if (refnum == (unsigned long)ST_NIL) {
                refnum = out_textstring_ix++;
                out_textstring_tab->add(out_string_pool->new_string(string),
                    (void*)refnum, false);
            }
            // write reference number
            if (!write_unsigned(refnum))
                return (false);
        }
        delete [] out_modal->text_string;
        out_modal->text_string = lstring::copy(string);
        out_modal->text_string_set = true;
    }
    delete [] string;

    if (NOT_MODAL(textlayer, out_layer.layer)) {
        if (!write_unsigned(out_layer.layer))
            return (false);
        MODAL_ASSIGN(textlayer, out_layer.layer);
    }
    if (NOT_MODAL(texttype, out_layer.datatype)) {
        if (!write_unsigned(out_layer.datatype))
            return (false);
        MODAL_ASSIGN(texttype, out_layer.datatype);
    }

    if (NOT_MODAL(text_x, x)) {
        if (!write_signed(x))
            return (false);
        MODAL_ASSIGN(text_x, x);
    }
    if (NOT_MODAL(text_y, y)) {
        if (!write_signed(y))
            return (false);
        MODAL_ASSIGN(text_y, y);
    }

    // write repetition
    if (out_use_repetition && !write_repetition())
        return (false);

    // maybe write Xic label property
    if (!(out_prp_mask & (OAS_PRPMSK_XIC_LBL | OAS_PRPMSK_ALL)) &&
            !write_label_property(scale(text->width), text->xform))
        return (false);

    // write properties
    return (write_elem_properties());
}


namespace {
    // Return true if ang is a multiple of 90 degrees, and set aa to the
    // direction code for PLACEMENT type 17.
    //
    bool
    is_manhattan(double ang, int *aa)
    {
        while (ang < 0)
            ang += 360.0;
        while (ang > 360.0)
            ang -= 360.0;
        if (fabs(ang) < 1e-12) {
            *aa = 0;
            return (true);
        }
        if (fabs(ang - 90.0) < 1e-12) {
            *aa = 1;
            return (true);
        }
        if (fabs(ang - 180.0) < 1e-12) {
            *aa = 2;
            return (true);
        }
        if (fabs(ang - 270.0) < 1e-12) {
            *aa = 3;
            return (true);
        }
        return (false);
    }
}


bool
oas_out::write_sref(const Instance *inst)
{
    if (!check_for_interrupt())
        return (false);

    if (out_cgd) {
        clear_property_queue();
        return (true);
    }

    if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
        clear_property_queue();

    if (inst->nx == 1 && inst->ny == 1) {
        // arrays aren't cached

        if (out_cache && out_cache->caching_srefs()) {
            bool ret = out_cache->cache_sref(inst, out_prpty);
            out_prpty = 0;
            if (ret)
                ret = out_cache->check_flush_sref();
            return (ret);
        }
    }

    if (out_in_struct) {
        // 17 placement-info-byte [reference-number | cellname-string]
        // [x][y][repetition]
        //
        // 18 placement-info-byte [reference-number | cellname-string]
        // [magnification] [angle] [x] [y] [repetition]

        const char *cellname = alias(inst->name);

        int aa = 0;
        bool is_manh = is_manhattan(inst->angle, &aa);

        // write PLACEMENT index, setup info byte
        unsigned char info_byte = 0;  // CNXYRAAF (17), CNXYRMAF (18),
        int x = scale(inst->origin.x);
        int y = scale(inst->origin.y);
        if (!out_modal->placement_cell_set ||
                strcmp(cellname, out_modal->placement_cell)) {
            info_byte |= 0x80;
            if (out_cellname_tab)
                info_byte |= 0x40;
        }
        if (NOT_MODAL(placement_x, inst->origin.x))
            info_byte |= 0x20;
        if (NOT_MODAL(placement_y, inst->origin.y))
            info_byte |= 0x10;

        bool use_18 = false;
        if (inst->magn != 1.0 || !is_manh) {
            use_18 = true;
            if (!write_unsigned(18))
                return (false);
            if (inst->magn != 1.0)
                info_byte |= 0x4;
            if (inst->angle != 0.0)
                info_byte |= 0x2;
        }
        else {
            if (!write_unsigned(17))
                return (false);
            if (aa)
                info_byte |= (aa << 1);
        }
        if (out_use_repetition || inst->nx > 1 || inst->ny > 1)
            info_byte |= 0x8;
        if (inst->reflection)
            info_byte |= 0x1;

        if (!write_char(info_byte))
            return (false);

        // write name or refnum
        if (!out_modal->placement_cell_set ||
                strcmp(cellname, out_modal->placement_cell)) {
            if (!out_cellname_tab) {
                // write cell name
                if (!write_n_string(cellname))
                    return (false);
            }
            else {
                unsigned long refnum = (unsigned long)SymTab::get(
                    out_cellname_tab, cellname);
                if (refnum == (unsigned long)ST_NIL) {
                    refnum = out_cellname_ix++;
                    out_cellname_tab->add(out_string_pool->new_string(cellname),
                        (void*)refnum, false);
                }
                if (!write_unsigned(refnum))
                    return (false);
            }
            delete [] out_modal->placement_cell;
            out_modal->placement_cell = lstring::copy(cellname);
            out_modal->placement_cell_set = true;
        }

        // write magnification, angle if necessary
        if (use_18) {
            if (inst->magn != 1.0) {
                if (!write_real(inst->magn))
                    return (false);
            }
            if (inst->angle != 0.0) {
                if (!write_real(inst->angle))
                    return (false);
            }
        }

        if (NOT_MODAL(placement_x, x)) {
            if (!write_signed(scale(inst->origin.x)))
                return (false);
            MODAL_ASSIGN(placement_x, x);
        }
        if (NOT_MODAL(placement_y, y)) {
            if (!write_signed(scale(inst->origin.y)))
                return (false);
            MODAL_ASSIGN(placement_y, y);
        }

        // write REPETITION record
        if (out_use_repetition) {
            if (!write_repetition())
                return (false);
        }
        else if (inst->nx > 1 || inst->ny > 1) {
            cTfmStack stk;
            stk.TPush();
            if (inst->reflection)
                stk.TMY();
            stk.TRotate(inst->ax, inst->ay);

            int dx1 = inst->dx;
            int dy1 = 0;
            int dx2 = 0;
            int dy2 = inst->dy;
            if (inst->nx > 1)
                stk.TPoint(&dx1, &dy1);
            if (inst->ny > 1)
                stk.TPoint(&dx2, &dy2);
            stk.TPop();

            if (inst->ny == 1) {
                if (dx1 >= 0 && abs(dy1) <= 1) {
                    // 2 x-dimension x-space
                    if (!write_unsigned(2))
                        return (false);
                    if (!write_unsigned(inst->nx - 2))
                        return (false);
                    if (!write_unsigned(scale(dx1)))
                        return (false);
                }
                else if (dy1 >= 0 && abs(dx1) <= 1) {
                    // 3 y-dimension y-space
                    if (!write_unsigned(3))
                        return (false);
                    if (!write_unsigned(inst->nx - 2))
                        return (false);
                    if (!write_unsigned(scale(dy1)))
                        return (false);
                }
                else {
                    // 9 dimension displacement
                    if (!write_unsigned(9))
                        return (false);
                    if (!write_unsigned(inst->nx - 2))
                        return (false);
                    if (!write_g_delta(scale(dx1), scale(dy1)))
                        return (false);
                }
            }
            else if (inst->nx == 1) {
                if (dx2 >= 0 && abs(dy2) <= 1) {
                    // 2 x-dimension x-space
                    if (!write_unsigned(2))
                        return (false);
                    if (!write_unsigned(inst->ny - 2))
                        return (false);
                    if (!write_unsigned(scale(dx2)))
                        return (false);
                }
                else if (dy2 >= 0 && abs(dx2) <= 1) {
                    // 3 y-dimension y-space
                    if (!write_unsigned(3))
                        return (false);
                    if (!write_unsigned(inst->ny - 2))
                        return (false);
                    if (!write_unsigned(scale(dy2)))
                        return (false);
                }
                else {
                    // 9 dimension displacement
                    if (!write_unsigned(9))
                        return (false);
                    if (!write_unsigned(inst->ny - 2))
                        return (false);
                    if (!write_g_delta(scale(dx2), scale(dy2)))
                        return (false);
                }
            }
            else {
                if (dx1 >= 0 && abs(dy1) <= 1 && abs(dx2) <= 1 && dy2 >= 0 ) {
                    // 1 x-dimension y-dimension x-space y-space
                    if (!write_unsigned(1))
                        return (false);
                    if (!write_unsigned(inst->nx - 2))
                        return (false);
                    if (!write_unsigned(inst->ny - 2))
                        return (false);
                    if (!write_unsigned(scale(dx1)))
                        return (false);
                    if (!write_unsigned(scale(dy2)))
                        return (false);
                }
                else
                if (abs(dx1) <= 1 && dy1 >= 0 && dx2 >= 0 && abs(dy2) <= 1) {
                    // 1 x-dimension y-dimension x-space y-space
                    if (!write_unsigned(1))
                        return (false);
                    if (!write_unsigned(inst->ny - 2))
                        return (false);
                    if (!write_unsigned(inst->nx - 2))
                        return (false);
                    if (!write_unsigned(scale(dx2)))
                        return (false);
                    if (!write_unsigned(scale(dy1)))
                        return (false);
                }
                else {
                    // n-dimension m-dimension n-displacement m-displacement
                    if (!write_unsigned(8))
                        return (false);
                    if (!write_unsigned(inst->nx - 2))
                        return (false);
                    if (!write_unsigned(inst->ny - 2))
                        return (false);
                    if (!write_g_delta(scale(dx1), scale(dy1)))
                        return (false);
                    if (!write_g_delta(scale(dx2), scale(dy2)))
                        return (false);
                }
            }
        }

        // write properties
        if (!write_cell_properties())
            return (false);
    }
    return (true);
}


bool
oas_out::write_endlib(const char*)
{
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    if (out_cgd)
        return (true);
    if (!write_tables())
        return (false);

    // 2 [table-offsets] padding-string validation-scheme
    // [validation-signature]

    int64_t posn = large_ftell(out_fp);

    // write END index
    if (!write_unsigned(2))
        return (false);

    if (out_offset_flag) {
        if (!write_offset_table())
            return (false);
    }
    else {
        int64_t tpos = large_ftell(out_fp);
        if (large_fseek(out_fp, out_offset_table, SEEK_SET) < 0) {
            Errs()->add_error("write_endlib: fseek failed.");
            return (false);
        }
        if (!write_offset_table())
            return (false);
        if (large_fseek(out_fp, tpos, SEEK_SET) < 0) {
            Errs()->add_error("write_endlib: fseek failed.");
            return (false);
        }
    }

    unsigned int tsize = large_ftell(out_fp) - posn;
    unsigned int vsize = 1;
    if (out_validation_type)
        vsize += 4;
    unsigned int padlen = 256 - tsize - vsize;
    // padlen includes string size (1 or 2 bytes)

    unsigned int sz = 1;
    if (usizeof(padlen - sz) != sz)
        sz = 2;
    padlen -= sz;

    // write string length
    if (!write_unsigned(padlen))
        return (false);

    // write string data (pdlen bytes)
    for (unsigned int i = 0; i < padlen; i++) {
        if (!write_char(0))
            return (false);
    }

    // write validation scheme
    if (!write_char(out_validation_type))
        return (false);

    if (out_validation_type) {
        fflush(out_fp);
        posn = large_ftell(out_fp);  // end of checksum
        unsigned int crc = 0;
        FILE *fp = large_fopen(out_filename, "rb");
        if (!fp) {
            Errs()->add_error(
                "write_endlib: open failed for validation.");
            return (false);
        }
        if (large_fseek(fp, out_start_offset, SEEK_SET) < 0) {
            Errs()->add_error(
                "write_endlib: fseek for validation failed.");
            fclose(fp);
            return (false);
        }
        int64_t bytes = posn - out_start_offset;
        const int BFSZ = 4096;
        unsigned char buf[BFSZ];
        if (out_validation_type == 1)
            crc = crc32(crc, 0, 0);
        while (bytes) {
            int tsz = bytes < BFSZ ? bytes : BFSZ;
            if ((int)fread(buf, 1, tsz, fp) != tsz) {
                Errs()->add_error(
                    "write_endlib: fread for validation failed.");
                fclose(fp);
                return (false);
            }
            bytes -= tsz;
            if (out_validation_type == 1)
                crc = crc32(crc, buf, tsz);
            else if (out_validation_type == 2) {
                for (int i = 0; i < tsz; i++)
                    crc += buf[i];
            }
        }
        fclose(fp);

        // save validation signature
        if (!write_char(crc & 0xff))
            return (false);
        if (!write_char((crc >> 8) & 0xff))
            return (false);
        if (!write_char((crc >> 16) & 0xff))
            return (false);
        if (!write_char((crc >> 24) & 0xff))
            return (false);
    }

    return (true);
}


bool
oas_out::write_info(Attribute*, const char*)
{
    return (true);
}
// End of virtual overrides


#define CTRAP_NOTYPE 1000

bool
oas_out::write_trapezoid(Zoid *z, bool vert)
{
    if (out_in_struct) {
        if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
            clear_property_queue();

        unsigned int type = CTRAP_NOTYPE;
        int delta_a, delta_b, x, y, w, h;
        if (vert) {
            delta_a = z->xll - z->xul;
            delta_b = z->xlr - z->xur;
            x = z->yl;
            y = z->minleft();
            w = z->yu - z->yl;
            h = z->maxright() - z->minleft();

            // check for CTRAPAZOID
            if ((delta_a == 0 || abs(delta_a) == w) &&
                    (delta_b == 0 || abs(delta_b) == w)) {
                if (delta_a == 0) {
                    if (delta_b == 0)
                        type = w == h ? 25 : 24;
                    else if (delta_b == w)
                        type = h > w ? 8 : 16;
                    else if (delta_b == -w)
                        type = h > w ? 9 : 18;
                }
                else if (delta_a == w) {
                    if (delta_b == 0)
                        type = h > w ? 11 : 19;
                    else if (delta_b == w)
                        type = 15;
                    else if (delta_b == -w)
                        type = h > 2*w ? 13 : 23;
                }
                else if (delta_a == -w) {
                    if (delta_b == 0)
                        type = h > w ? 10 : 17;
                    else if (delta_b == w)
                        type = h > 2*w ? 12 : 22;
                    else if (delta_b == -w)
                        type = 14;
                }
            }
        }
        else {
            delta_a = z->xul - z->xll;
            delta_b = z->xur - z->xlr;
            x = z->minleft();
            y = z->yl;
            w = z->maxright() - z->minleft();
            h = z->yu - z->yl;

            // check for CTRAPAZOID
            if ((delta_a == 0 || abs(delta_a) == h) &&
                    (delta_b == 0 || abs(delta_b) == h)) {
                if (delta_a == 0) {
                    if (delta_b == 0)
                        type = w == h ? 25 : 24;
                    else if (delta_b == h)
                        type = w > h ? 1 : 17;
                    else if (delta_b == -h)
                        type = w > h ? 0 : 16;
                }
                else if (delta_a == h) {
                    if (delta_b == 0)
                        type = w > h ? 2 : 18;
                    else if (delta_b == h)
                        type = 6;
                    else if (delta_b == -h)
                        type = w > 2*h ? 4 : 20;
                }
                else if (delta_a == -h) {
                    if (delta_b == 0)
                        type = w > h ? 3 : 19;
                    else if (delta_b == h)
                        type = w > 2*h ? 5 : 21;
                    else if (delta_b == -h)
                        type = 7;
                }
            }
        }

        if (type != CTRAP_NOTYPE) {

            // write CTRAPEZOID index
            // 26 ctrap-info-byte [layer-number] [datatype-number]
            //  [ctrap-type] [width] [height] [x] [y] [repetition]
            if (!write_unsigned(26))
                return (false);

            // write info byte
            unsigned char info_byte = 0x0;  // TWHXYRDL
            if (NOT_MODAL(ctrapezoid_type, type))
                info_byte |= 0x80;
            if (type != 20 && type != 21 &&
                    (NOT_MODAL(geometry_w, (unsigned int)w)))
                info_byte |= 0x40;
            if ((type < 16 || type == 20 || type == 21 || type == 24) &&
                    (NOT_MODAL(geometry_h, (unsigned int)h)))
                info_byte |= 0x20;
            if (NOT_MODAL(geometry_x, x))
                info_byte |= 0x10;
            if (NOT_MODAL(geometry_y, y))
                info_byte |= 0x8;
            if (out_use_repetition)
                info_byte |= 0x4;
            if (NOT_MODAL(datatype, out_layer.datatype))
                info_byte |= 0x2;
            if (NOT_MODAL(layer, out_layer.layer))
                info_byte |= 0x1;
            if (!write_char(info_byte))
                return (false);

            if (NOT_MODAL(layer, out_layer.layer)) {
                if (!write_unsigned(out_layer.layer))
                    return (false);
                MODAL_ASSIGN(layer, out_layer.layer);
            }
            if (NOT_MODAL(datatype, out_layer.datatype)) {
                if (!write_unsigned(out_layer.datatype))
                    return (false);
                MODAL_ASSIGN(datatype, out_layer.datatype);
            }

            // write type
            if (NOT_MODAL(ctrapezoid_type, type)) {
                if (!write_unsigned(type))
                    return (false);
                MODAL_ASSIGN(ctrapezoid_type, type);
            }

            // write width, height
            if (type != 20 && type != 21 &&
                    (NOT_MODAL(geometry_w, (unsigned int)w))) {
                if (!write_unsigned(scale(w)))
                    return (false);
                MODAL_ASSIGN(geometry_w, (unsigned int)w);
            }
            if ((type < 16 || type == 20 || type == 21 || type == 24) &&
                    (NOT_MODAL(geometry_h, (unsigned int)h))) {
                if (!write_unsigned(scale(h)))
                    return (false);
                MODAL_ASSIGN(geometry_h, (unsigned int)h);
            }

            // write x, y
            if (NOT_MODAL(geometry_x, x)) {
                if (!write_signed(scale(x)))
                    return (false);
                MODAL_ASSIGN(geometry_x, x);
            }
            if (NOT_MODAL(geometry_y, y)) {
                if (!write_signed(scale(y)))
                    return (false);
                MODAL_ASSIGN(geometry_y, y);
            }

            // write repetition
            if (out_use_repetition && !write_repetition())
                return (false);

            // write properties
            if (!write_elem_properties())
                return (false);

            if (type == 20 || type == 21)
                MODAL_ASSIGN(geometry_w, 2*(unsigned int)h);
            if (type == 22 || type == 23)
                MODAL_ASSIGN(geometry_h, 2*(unsigned int)w);
            if ((type >= 16 && type <= 19) || type == 25)
                MODAL_ASSIGN(geometry_h, (unsigned int)w);
        }
        else {
            // write TRAPEZOID index
            if (delta_a && delta_b) {
                // 23 trap-info-byte [layer-number] [datatype-number]
                //  [width] [height] delta-a delta-b [x] [y] [repetition]
                if (!write_unsigned(23))
                    return (false);
            }
            else if (delta_a) {
                // 24 trap-info-byte [layer-number] [datatype-number]
                //  [width] [height] delta-a [x] [y] [repetition]
                if (!write_unsigned(24))
                    return (false);
            }
            else {
                // 25 trap-info-byte [layer-number] [datatype-number]
                //  [width] [height] delta-b [x] [y] [repetition]
                if (!write_unsigned(25))
                    return (false);
            }

            // write info byte
            unsigned char info_byte = 0x0;  // OWHXYRDL
            if (vert)
                info_byte |= 0x80;
            if (NOT_MODAL(geometry_w, (unsigned int)w))
                info_byte |= 0x40;
            if (NOT_MODAL(geometry_h, (unsigned int)h))
                info_byte |= 0x20;
            if (NOT_MODAL(geometry_x, x))
                info_byte |= 0x10;
            if (NOT_MODAL(geometry_y, y))
                info_byte |= 0x8;
            if (out_use_repetition)
                info_byte |= 0x4;
            if (NOT_MODAL(datatype, out_layer.datatype))
                info_byte |= 0x2;
            if (NOT_MODAL(layer, out_layer.layer))
                info_byte |= 0x1;
            if (!write_char(info_byte))
                return (false);

            if (NOT_MODAL(layer, out_layer.layer)) {
                if (!write_unsigned(out_layer.layer))
                    return (false);
                MODAL_ASSIGN(layer, out_layer.layer);
            }
            if (NOT_MODAL(datatype, out_layer.datatype)) {
                if (!write_unsigned(out_layer.datatype))
                    return (false);
                MODAL_ASSIGN(datatype, out_layer.datatype);
            }

            // write width, height
            if (NOT_MODAL(geometry_w, (unsigned int)w)) {
                if (!write_unsigned(scale(w)))
                    return (false);
                MODAL_ASSIGN(geometry_w, (unsigned int)w);
            }
            if (NOT_MODAL(geometry_h, (unsigned int)h)) {
                if (!write_unsigned(scale(h)))
                    return (false);
                MODAL_ASSIGN(geometry_h, (unsigned int)h);
            }

            // write deltas
            if (delta_a) {
                if (!write_signed(scale(delta_a)))
                    return (false);
            }
            if (delta_b) {
                if (!write_signed(scale(delta_b)))
                    return (false);
            }

            // write x, y
            if (NOT_MODAL(geometry_x, x)) {
                if (!write_signed(scale(x)))
                    return (false);
                MODAL_ASSIGN(geometry_x, x);
            }
            if (NOT_MODAL(geometry_y, y)) {
                if (!write_signed(scale(y)))
                    return (false);
                MODAL_ASSIGN(geometry_y, y);
            }

            // write repetition
            if (out_use_repetition && !write_repetition())
                return (false);

            // write properties
            if (!write_elem_properties())
                return (false);
        }
    }
    return (true);
}


// If this is called before write(), the file will use reference numbers
// for strings.
//
void
oas_out::init_tables(DisplayMode mode)
{
    if (mode == Physical) {
        if (FIO()->IsStripForExport())
            // Physical only, put offset table in END record.
            out_offset_flag = true;
        else
            // May write both physical and electrical (two sequential
            // OASIS databases).  The offset table *must* be in the START
            // record.
            out_offset_flag = false;

        out_cellname_ix = 0;
        out_textstring_ix = 0;
        out_propname_ix = 0;
        out_propstring_ix = 0;
        out_layername_ix = 0;
        out_xname_ix = 0;

        delete out_string_pool;
        out_string_pool = new stbuf_t;
        delete out_cellname_tab;
        out_cellname_tab = new SymTab(false, false);
        delete out_textstring_tab;
        out_textstring_tab = new SymTab(false, false);
        delete out_propname_tab;
        out_propname_tab = new SymTab(false, false);
        delete out_propstring_tab;
        out_propstring_tab = new SymTab(false, false);
        delete out_layername_tab;
        out_layername_tab = new SymTab(false, false);
        delete out_xname_tab;
        out_xname_tab = new SymTab(false, false);
    }
    else {
        // No tables used in electrical mode, simplifies reading.
        out_use_tables = false;
        out_offset_flag = false;

        out_cellname_ix = 0;
        out_textstring_ix = 0;
        out_propname_ix = 0;
        out_propstring_ix = 0;
        out_layername_ix = 0;
        out_xname_ix = 0;

        delete out_string_pool;
        out_string_pool = 0;
        delete out_cellname_tab;
        out_cellname_tab = 0;
        delete out_textstring_tab;
        out_textstring_tab = 0;
        delete out_propname_tab;
        out_propname_tab = 0;
        delete out_propstring_tab;
        out_propstring_tab = 0;
        delete out_layername_tab;
        out_layername_tab = 0;
        delete out_xname_tab;
        out_xname_tab = 0;
    }
}


void
oas_out::enable_cache(bool set)
{
    delete out_cache;
    out_cache = 0;
    if (set) {
        out_cache = new oas_cache(this);
        out_cache->setup_repetition(out_rep_args, out_no_gcd_check);
    }
}


inline bool
oas_out::write_char(int c)
{
    if (out_cgd) {
        if (out_lmux) {
            if (!out_lmux->put_byte(c)) {
                Errs()->add_error("lmux->put_byte failed.");
                Errs()->add_error("write error, file system full?");
                return (false);
            }
            out_byte_count++;
        }
        return (true);
    }
    if (out_compressing) {
        if (out_zfile) {
            if (out_zfile->zio_putc(c) == EOF) {
                Errs()->add_error("zio_putc failed.");
                Errs()->add_error("write error, file system full?");
                return (false);
            }
        }
        else {
            int nbytes = out_byte_count - out_comp_start;
            if (nbytes == CGD_COMPR_BUFSIZE ||
                    out_use_compression == OAScompForce) {
                // Overflow (or forcing all), enable the "real"
                // compression.  The compressed output goes to a temp
                // file, which is copied to the output file after
                // writing the CBLOCK record, which can't be done
                // until we know the compressed and uncompressed
                // sizes.

                if (!out_tmp_fp) {
                    out_tmp_fname = filestat::make_temp("cd");
                    out_tmp_fp = large_fopen(out_tmp_fname, "wb+");
                    if (!out_tmp_fp) {
                        Errs()->add_error(
                            "write_char: open failed for temp file.");
                        return (false);
                    }
                }
                else
                    rewind(out_tmp_fp);
                out_zfile = zio_stream::zio_open(out_tmp_fp, "wb");
                if (!out_zfile) {
                    Errs()->add_error("write_char: open failed.");
                    return (false);
                }
                for (int i = 0; i < nbytes; i++) {
                    if (out_zfile->zio_putc(out_compr_buf[i]) == EOF) {
                        Errs()->add_error("zio_putc failed.");
                        Errs()->add_error("write error, file system full?");
                        return (false);
                    }
                }
                if (out_zfile->zio_putc(c) == EOF) {
                    Errs()->add_error("zio_putc failed.");
                    Errs()->add_error("write error, file system full?");
                    return (false);
                }
            }
            else {
                if (!out_compr_buf)
                    out_compr_buf = new unsigned char[CGD_COMPR_BUFSIZE];
                out_compr_buf[nbytes] = c;
            }
        }
    }
    else {
        if (putc(c, out_fp) == EOF) {
            Errs()->sys_error("write");
            Errs()->add_error("write error, file system full?");
            return (false);
        }
    }
    out_byte_count++;
    return (true);
}


bool
oas_out::write_unsigned(unsigned i)
{
    unsigned char b = i & 0x7f;
    i >>= 7;        // >> 7
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 14
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 21
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 28
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0xf;    // 4 bits left
    return (write_char(b));
}


bool
oas_out::write_unsigned64(uint64_t i)
{
    unsigned char b = i & 0x7f;
    i >>= 7;        // >> 7
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 14
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 21
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 28
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 35
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 42
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 49
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 56
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 63
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x1;    // 1 bit left
    return (write_char(b));
}


bool
oas_out::write_signed(int i)
{
    bool neg = false;
    if (i < 0) {
        neg = true;
        i = -i;
    }
    unsigned char b = i & 0x3f;
    b <<= 1;
    if (neg)
        b |= 1;
    i >>= 6;        // >> 6
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 13
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 20
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 27
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0xf;    // 4 bits left (sign bit excluded)
    return (write_char(b));
}


bool
oas_out::write_delta(int nbits, int bits, unsigned int i)
{
    if (nbits > 4)
        return (false);
    unsigned char b = bits;
    unsigned char t = i << nbits;
    b |= (t & 0x7f);
    int sh = 8 - nbits - 1;
    i >>= sh;
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    return (write_char(b));
}


bool
oas_out::write_g_delta(int x, int y)
{
    if (x == 0 || y == 0 || abs(x) == abs(y)) {
        // type 1
        unsigned char b = 0;
        if (abs(x) == abs(y)) {
            if (!x) {
                if (!write_delta(4, 0, 0))
                    return (false);
                return (true);
            }
            b |= 0x8;
            if (y < 0) {
                if (x < 0)
                    // southwest
                    b |= 0x4;
                else
                    // southeast
                    b |= 0x6;
            }
            else {
                if (x < 0) {
                    // northwest
                    b |= 0x2;
                }
            }
        }
        else if (x) {
            if (x < 0)
                b |= 0x4;
        }
        else {
            if (y < 0)
                b |= 0x6;
            else
                b |= 0x2;
        }
        if (!write_delta(4, b, x ? abs(x) : abs(y)))
            return (false);
    }
    else {
        // type 2
        unsigned char b = 0x1;
        if (x < 0)
            b |= 2;
        if (!write_delta(2, b, abs(x)))
            return (false);
        if (!write_signed(y))
            return (false);
    }
    return (true);
}


namespace {
    // Return true if the points form an x/y alternating Manhattan
    // sequence.
    //
    bool
    check_manhattan(Point *points, int numpts)
    {
        int lastj = (points[0].x == points[numpts-2].x) +
                ((points[0].y == points[numpts-2].y) << 1);
        if (!lastj || lastj == 3)
            return (false);
        for (int i = 1; i < numpts; i++) {
            int j = (points[i].x == points[i-1].x) +
                ((points[i].y == points[i-1].y) << 1);
            if (!j || j == 3)
                return (false);
            if (lastj && lastj == j)
                return (false);
            lastj = j;
        }
        return (true);
    }


    // Return true if the points form angles that are multiples of 45
    // degrees.
    //
    bool
    check_oct(Point *points, int numpts)
    {
        int dx = points[0].x - points[numpts-2].x;
        int dy = points[0].y - points[numpts-2].y;
        if (dx && dy && abs(dx) != abs(dy))
            return (false);
        for (int i = 1; i < numpts; i++) {
            dx = points[i].x - points[i-1].x;
            dy = points[i].y - points[i-1].y;
            if (dx && dy && abs(dx) != abs(dy))
                return (false);
        }
        return (true);
    }
}


// Write a point list.
bool
oas_out::write_point_list(Point *points, int numpts, bool is_polygon)
{
    if (!is_polygon && numpts <= 1) {
        if (!write_unsigned(0))
            return (false);
        if (!write_unsigned(0))
            return (false);
        return (true);
    }
    if (check_manhattan(points, numpts)) {
        if (is_polygon) {
            numpts -= 2;
            if (numpts < 3 || !(numpts & 1)) {
                Errs()->add_error(
                "write_point_list: bad count %d for type %d poly point list.",
                numpts-1, (points[1].x == points[0].x));
                return (false);
            }
        }
        if (points[1].y == points[0].y) {
            // Alternating Manhattan, start horizontal.
            if (!write_unsigned(0))
                return (false);
            if (!write_unsigned(numpts - 1))
                return (false);
            for (int i = 1; i < numpts; i++) {
                if (i & 1) {
                    if (!write_signed(scale(points[i].x - points[i-1].x)))
                        return (false);
                }
                else {
                    if (!write_signed(scale(points[i].y - points[i-1].y)))
                        return (false);
                }
            }
        }
        else {
            // Alternating Manhattan, start vertical.
            if (!write_unsigned(1))
                return (false);
            if (!write_unsigned(numpts - 1))
                return (false);
            for (int i = 1; i < numpts; i++) {
                if (i & 1) {
                    if (!write_signed(scale(points[i].y - points[i-1].y)))
                        return (false);
                }
                else {
                    if (!write_signed(scale(points[i].x - points[i-1].x)))
                        return (false);
                }
            }
        }
    }
    else if (check_oct(points, numpts)) {
        if (is_polygon) {
            numpts--;
            if (numpts < 3) {
                Errs()->add_error(
                "write_point_list: bad count %d for type %d poly point list.",
                numpts-1, 3);
                return (false);
            }
        }
        if (!write_unsigned(3))
            return (false);
        if (!write_unsigned(numpts - 1))
            return (false);
        for (int i = 1; i < numpts; i++) {
            int dx = points[i].x - points[i-1].x;
            int dy = points[i].y - points[i-1].y;
            unsigned int m, d;
            if (dx == 0) {
                m = abs(dy);
                if (dy > 0)
                    d = 1;  // n
                else
                    d = 3;  // s
            }
            else if (dy == 0) {
                m = abs(dx);
                if (dx > 0)
                    d = 0;  // e
                else
                    d = 2;  // w
            }
            else if (dx > 0) {
                m = dx;
                if (dy > 0)
                    d = 4;  // ne
                else
                    d = 7;  // se
            }
            else {
                m = -dx;
                if (dy > 0)
                    d = 5;  // nw
                else
                    d = 6;  // sw
            }
            if (!write_delta(3, d, m))
                return (false);
        }
    }
    else {
        if (is_polygon) {
            numpts--;
            if (numpts < 3) {
                Errs()->add_error(
                "write_point_list: bad count %d for type %d poly point list.",
                numpts-1, 4);
                return (false);
            }
        }
        if (!write_unsigned(4))
            return (false);
        if (!write_unsigned(numpts - 1))
            return (false);
        for (int i = 1; i < numpts; i++) {
            if (!write_g_delta(scale(points[i].x - points[i-1].x),
                    scale(points[i].y - points[i-1].y)))
                return (false);
        }
    }
    return (true);
}


bool
oas_out::write_real(double val)
{
    // ieee double
    // first byte is lsb of mantissa
    if (!write_unsigned(7))
        return (false);
    union { double n; unsigned char b[sizeof(double)]; } u;
    u.n = 1.0;
    bool ret;
    if (u.b[0] == 0) {
        // machine is little-endian, write bytes in order
        u.n = val;
        write_char(u.b[0]);
        write_char(u.b[1]);
        write_char(u.b[2]);
        write_char(u.b[3]);
        write_char(u.b[4]);
        write_char(u.b[5]);
        write_char(u.b[6]);
        ret = write_char(u.b[7]);   
    }
    else {
        // machine is big-endian, write bytes in reverse order
        u.n = val;
        write_char(u.b[7]);
        write_char(u.b[6]);
        write_char(u.b[5]);
        write_char(u.b[4]);
        write_char(u.b[3]);
        write_char(u.b[2]);
        write_char(u.b[1]);
        ret = write_char(u.b[0]);
    }
    return (ret);
}


bool
oas_out::write_n_string(const char *string)
{
    if (!string)
        return (false);
    unsigned int len = strlen(string);
    if (!write_unsigned(len))
        return (false);
    const char *s = string;
    while (len--) {
        if (*s < 0x21 || *s > 0x7e) {
            Errs()->add_error(
                "write_n_string: forbidden character '0x%x'.", *s);
            return (false);
        }
        else if (!write_char(*s))
            return (false);
        s++;
    }
    return (true);
}


bool
oas_out::write_a_string(const char *string)
{
    if (!string)
        return (false);

    // This is bad, the OASIS a-string should allow newlines and tabs. 
    // We will do some silent substitution here.
    // Also, we will strip any trailing newline chars.

    unsigned int len = strlen(string);
    const char *e = string + len - 1;
    while (e >= string && (*e == '\n' || *e == '\r')) {
        e--;
        len--;
    }
    e++;

    for (const char *s = string; s != e; s++) {
        if (*s >= 0x20)
            continue;
        if (*s == '\n' || *s == '\r' || *s == '\t')
            len++;
    }
    if (!write_unsigned(len))
        return (false);

    for (const char *s = string; s != e; s++) {
        if (*s < 0x20 || *s > 0x7e) {
            if (*s == '\n') {
                if (!write_char('\\'))
                    return (false);
                if (!write_char('n'))
                    return (false);
            }
            else if (*s == '\r') {
                if (!write_char('\\'))
                    return (false);
                if (!write_char('r'))
                    return (false);
            }
            else if (*s == '\t') {
                if (!write_char('\\'))
                    return (false);
                if (!write_char('t'))
                    return (false);
            }
            else {
                FIO()->ifPrintCvLog(IFLOG_WARN,
        "write_a_string: forbidden character '0x%x' replaced with space", *s);
                if (!write_char(' '))
                    return (false);
            }
        }
        else if (!write_char(*s))
            return (false);
    }
    return (true);
}


bool
oas_out::write_b_string(const char *string, unsigned int len)
{
    if (!string || !len)
        return (false);
    if (!write_unsigned(len))
        return (false);
    const char *s = string;
    while (len--) {
        if (!write_char(*s))
            return (false);
        s++;
    }
    return (true);
}


bool
oas_out::write_object_prv(const CDo *odesc)
{
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
        ret = text.set(&la, out_mode, Foas);
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
oas_out::write_cell_properties()
{
    if (out_cgd)
        return (true);
    if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
        return (true);

    unsigned int pcnt = 0;
    for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
        pcnt++;
    if (!pcnt)
        return (true);

    if (!write_unsigned(28))
        return (false);

    pcnt *= 2;
    const char *pname = "XIC_PROPERTIES";
    // All properties are in a list (pname) as number,value ...

    unsigned char info_byte = 0x4;  // UUUUVCNS
    if (pcnt < 15)
        info_byte |= (pcnt << 4);
    else
        info_byte |= 0xf0;

    if (!out_propname_tab) {
        if (!write_char(info_byte))
            return (false);
        if (!write_n_string(pname))
            return (false);
    }
    else {
        info_byte |= 2;  // N = 1
        if (!write_char(info_byte))
            return (false);
        unsigned long refnum = (unsigned long)SymTab::get(
            out_propname_tab, pname);
        if (refnum == (unsigned long)ST_NIL) {
            refnum = out_propname_ix++;
            out_propname_tab->add(out_string_pool->new_string(pname),
                (void*)refnum, false);
        }
        if (!write_unsigned(refnum))
            return (false);
    }
    if (pcnt >= 15) {
        if (!write_unsigned(pcnt))
            return (false);
    }

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        CDp *px = pd->dup();
        if (!px)
            continue;
        px->scale(out_scale, out_phys_scale, out_mode);
        char *str = lstring::copy(px->string());
        delete px;
        GCarray<char*> gc_str(str);

        if (!write_unsigned(8))
            return (false);
        if (!write_unsigned(pd->value()))
            return (false);

        if (!out_propstring_tab) {
            if (!write_unsigned(10))
                return (false);
            if (!write_a_string(str))
                return (false);
        }
        else {
            if (!write_unsigned(13))
                return (false);
            unsigned long refnum = (unsigned long)SymTab::get(
                out_propstring_tab, str);
            if (refnum == (unsigned long)ST_NIL) {
                refnum = out_propstring_ix++;
                out_propstring_tab->add(out_string_pool->new_string(str),
                    (void*)refnum, false);
            }
            if (!write_unsigned(refnum))
                return (false);
        }
    }
    return (true);
}


// GDSII TEXT record number
#ifndef II_TEXT
#define II_TEXT         12
#endif

bool
oas_out::write_elem_properties()
{
    if (out_cgd)
        return (true);
    if ((out_prp_mask & OAS_PRPMSK_ALL) && out_mode == Physical)
        return (true);

    unsigned int pcnt = 0;
    CDp *pp = 0, *pn;
    for (CDp *pd = out_prpty; pd; pd = pn) {
        pn = pd->next_prp();
        if (pd->value() == GDSII_PROPERTY_BASE + II_TEXT &&
                (out_prp_mask & OAS_PRPMSK_GDS_LBL)) {
            // Strip out the GDSII text property.
            if (pp)
                pp->set_next_prp(pn);
            else
                out_prpty = pn;
            delete pd;
            continue;
        }
        pp = pd;
        pcnt++;
    }
    if (!pcnt)
        return (true);

    if (!write_unsigned(28))
        return (false);

    pcnt *= 2;
    const char *pname = "XIC_PROPERTIES";
    // All properties are in a list (pname) as number,value ...

    unsigned char info_byte = 0x4;  // UUUUVCNS
    if (pcnt < 15)
        info_byte |= (pcnt << 4);
    else
        info_byte |= 0xf0;

    if (!out_propname_tab) {
        if (!write_char(info_byte))
            return (false);
        if (!write_n_string(pname))
            return (false);
    }
    else {
        info_byte |= 2;  // N = 1
        if (!write_char(info_byte))
            return (false);
        unsigned long refnum = (unsigned long)SymTab::get(
            out_propname_tab, pname);
        if (refnum == (unsigned long)ST_NIL) {
            refnum = out_propname_ix++;
            out_propname_tab->add(out_string_pool->new_string(pname),
                (void*)refnum, false);
        }
        if (!write_unsigned(refnum))
            return (false);
    }
    if (pcnt >= 15) {
        if (!write_unsigned(pcnt))
            return (false);
    }

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        if (!write_unsigned(8))
            return (false);
        if (!write_unsigned(pd->value()))
            return (false);

        if (!out_propstring_tab) {
            if (!write_unsigned(10))
                return (false);
            if (!write_a_string(pd->string()))
                return (false);
        }
        else {
            if (!write_unsigned(13))
                return (false);
            unsigned long refnum = (unsigned long)SymTab::get(
                out_propstring_tab, pd->string());
            if (refnum == (unsigned long)ST_NIL) {
                refnum = out_propstring_ix++;
                out_propstring_tab->add(
                    out_string_pool->new_string(pd->string()),
                    (void*)refnum, false);
            }
            if (!write_unsigned(refnum))
                return (false);
        }
    }
    return (true);
}


// This is not currently used.
bool
oas_out::write_offset_std_prpty(uint64_t offset)
{
    if (!write_unsigned(28))
        return (false);
    const char *pname = "S_CELL_OFFSET";
    const int pcnt = 1;
    unsigned char info_byte = 0x5;  // UUUUVCNS
    info_byte |= (pcnt << 4);
    if (!out_propname_tab) {
        if (!write_char(info_byte))
            return (false);
        if (!write_n_string(pname))
            return (false);
    }
    else {
        info_byte |= 2;  // N = 1
        if (!write_char(info_byte))
            return (false);
        unsigned long refnum = (unsigned long)SymTab::get(
            out_propname_tab, pname);
        if (refnum == (unsigned long)ST_NIL) {
            refnum = out_propname_ix++;
            out_propname_tab->add(out_string_pool->new_string(pname),
                (void*)refnum, false);
        }
        if (!write_unsigned(refnum))
            return (false);
    }

    if (!write_unsigned(8))
        return (false);
    if (!write_unsigned64(offset))
        return (false);
    return (true);
}


bool
oas_out::write_label_property(unsigned int width, unsigned int xform)
{
    // This is added to all labels to pass the Xic presentation
    // attributes.  The property name is "XIC_LABEL" and it consists
    // of two unsigned numbers:  width and xform.

    if (!write_unsigned(28))
        return (false);
    const char *pname = "XIC_LABEL";
    const int pcnt = 2;
    unsigned char info_byte = 0x4;  // UUUUVCNS
    info_byte |= (pcnt << 4);
    if (!out_propname_tab) {
        if (!write_char(info_byte))
            return (false);
        if (!write_n_string(pname))
            return (false);
    }
    else {
        info_byte |= 2;  // N = 1
        if (!write_char(info_byte))
            return (false);
        unsigned long refnum = (unsigned long)SymTab::get(
            out_propname_tab, pname);
        if (refnum == (unsigned long)ST_NIL) {
            refnum = out_propname_ix++;
            out_propname_tab->add(out_string_pool->new_string(pname),
                (void*)refnum, false);
        }
        if (!write_unsigned(refnum))
            return (false);
    }

    if (!write_unsigned(8))
        return (false);
    if (!write_unsigned(width))
        return (false);
    if (!write_unsigned(8))
        return (false);
    if (!write_unsigned(xform))
        return (false);
    return (true);
}


bool
oas_out::write_rounded_end_property()
{
    // This is added to PATHTYPE=1 wires to pass the fact that a
    // rounded end is desired.

    if (!write_unsigned(28))
        return (false);
    const char *pname = "XIC_ROUNDED_END";
    unsigned char info_byte = 0x4;  // UUUUVCNS

    if (!out_propname_tab) {
        if (!write_char(info_byte))
            return (false);
        if (!write_n_string(pname))
            return (false);
    }
    else {
        info_byte |= 2;  // N = 1
        if (!write_char(info_byte))
            return (false);
        unsigned long refnum = (unsigned long)SymTab::get(
            out_propname_tab, pname);
        if (refnum == (unsigned long)ST_NIL) {
            refnum = out_propname_ix++;
            out_propname_tab->add(out_string_pool->new_string(pname),
                (void*)refnum, false);
        }
        if (!write_unsigned(refnum))
            return (false);
    }
    return (true);
}


// Write the offset table, must be at proper location in START or END
// record.
//
bool
oas_out::write_offset_table()
{
    // CELLNAME
    if (out_cellname_off) {
        if (!write_unsigned(1))
            return (false);
        if (!write_unsigned64(out_cellname_off))
            return (false);
        out_cellname_off = 0;
    }
    else {
        if (!write_unsigned(0))
            return (false);
        if (!write_unsigned(0))
            return (false);
    }

    // TEXTSTRING
    if (out_textstring_off) {
        if (!write_unsigned(1))
            return (false);
        if (!write_unsigned64(out_textstring_off))
            return (false);
        out_textstring_off = 0;
    }
    else {
        if (!write_unsigned(0))
            return (false);
        if (!write_unsigned(0))
            return (false);
    }

    // PROPNAME
    if (out_propname_off) {
        if (!write_unsigned(1))
            return (false);
        if (!write_unsigned64(out_propname_off))
            return (false);
        out_propname_off = 0;
    }
    else {
        if (!write_unsigned(0))
            return (false);
        if (!write_unsigned(0))
            return (false);
    }

    // PROPSTRING
    if (out_propstring_off) {
        if (!write_unsigned(1))
            return (false);
        if (!write_unsigned64(out_propstring_off))
            return (false);
        out_propstring_off = 0;
    }
    else {
        if (!write_unsigned(0))
            return (false);
        if (!write_unsigned(0))
            return (false);
    }

    // LAYERNAME
    if (out_layername_off) {
        if (!write_unsigned(1))
            return (false);
        if (!write_unsigned64(out_layername_off))
            return (false);
        out_layername_off = 0;
    }
    else {
        if (!write_unsigned(0))
            return (false);
        if (!write_unsigned(0))
            return (false);
    }

    // XNAME
    if (out_xname_off) {
        if (!write_unsigned(1))
            return (false);
        if (!write_unsigned64(out_xname_off))
            return (false);
        out_xname_off = 0;
    }
    else {
        if (!write_unsigned(0))
            return (false);
        if (!write_unsigned(0))
            return (false);
    }
    return (true);
}


namespace {
    struct t_ary
    {
        t_ary(unsigned int sz)
            {
                ary = new const char*[sz];
                memset(ary, 0, sz*sizeof(const char*));
            }
        ~t_ary() { delete [] ary; }

        const char **ary;
    };
}


bool
oas_out::write_tables()
{
    if (out_cgd)
        return (true);
    if (out_cellname_tab && out_cellname_tab->allocated() > 0) {
        // '3' cellname-string
        out_cellname_off = large_ftell(out_fp);
        unsigned int sz =  out_cellname_tab->allocated();
        t_ary ta(sz);
        SymTabGen gen(out_cellname_tab, false);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            unsigned long ix = (unsigned long)h->stData;
            ta.ary[ix] = h->stTag;
        }
        if (!begin_compression(0))
            return (false);
        for (unsigned int i = 0; i < sz; i++) {
            if (!ta.ary[i])
                // uh-oh
                return (false);
            if (!write_char(3))
                return (false);
            if (!write_n_string(ta.ary[i]))
                return (false);
        }
        if (!end_compression())
            return (false);
    }
    if (out_textstring_tab && out_textstring_tab->allocated() > 0) {
        // '5' text-string
        out_textstring_off = large_ftell(out_fp);
        unsigned int sz =  out_textstring_tab->allocated();
        t_ary ta(sz);
        SymTabGen gen(out_textstring_tab, false);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            unsigned long ix = (unsigned long)h->stData;
            ta.ary[ix] = h->stTag;
        }
        if (!begin_compression(0))
            return (false);
        for (unsigned int i = 0; i < sz; i++) {
            if (!ta.ary[i])
                // uh-oh
                return (false);
            if (!write_char(5))
                return (false);
            if (!write_a_string(ta.ary[i]))
                return (false);
        }
        if (!end_compression())
            return (false);
    }
    if (out_propname_tab && out_propname_tab->allocated() > 0) {
        // '7' propname-string
        out_propname_off = large_ftell(out_fp);
        unsigned int sz =  out_propname_tab->allocated();
        t_ary ta(sz);
        SymTabGen gen(out_propname_tab, false);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            unsigned long ix = (unsigned long)h->stData;
            ta.ary[ix] = h->stTag;
        }
        if (!begin_compression(0))
            return (false);
        for (unsigned int i = 0; i < sz; i++) {
            if (!ta.ary[i])
                // uh-oh
                return (false);
            if (!write_char(7))
                return (false);
            if (!write_n_string(ta.ary[i]))
                return (false);
        }
        if (!end_compression())
            return (false);
    }
    if (out_propstring_tab && out_propstring_tab->allocated() > 0) {
        // '9' prop-string
        out_propstring_off = large_ftell(out_fp);
        unsigned int sz =  out_propstring_tab->allocated();
        t_ary ta(sz);
        SymTabGen gen(out_propstring_tab, false);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            unsigned long ix = (unsigned long)h->stData;
            ta.ary[ix] = h->stTag;
        }
        if (!begin_compression(0))
            return (false);
        for (unsigned int i = 0; i < sz; i++) {
            if (!ta.ary[i])
                // uh-oh
                return (false);
            if (!write_char(9))
                return (false);
            if (!write_a_string(ta.ary[i]))
                return (false);
        }
        if (!end_compression())
            return (false);
    }
    if (out_layername_tab && out_layername_tab->allocated() > 0) {
        // '11' layer-name layer-interval datatype-interval
        out_layername_off = large_ftell(out_fp);
        unsigned int sz =  out_layername_tab->allocated();
        t_ary ta(sz);
        SymTabGen gen(out_layername_tab, false);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            unsigned long ix = (unsigned long)h->stData;
            ta.ary[ix] = h->stTag;
        }
        if (!begin_compression(0))
            return (false);
        for (unsigned int i = 0; i < sz; i++) {
            if (!ta.ary[i])
                // uh-oh
                return (false);
            if (!write_unsigned(11))
                return (false);
            if (!write_unsigned(0))
                // layer-interval
                return (false);
            if (!write_unsigned(0))
                // datatype-interval
                return (false);
            if (!write_n_string(ta.ary[i]))
                return (false);
        }
        if (!end_compression())
            return (false);
    }
    if (out_xname_tab && out_xname_tab->allocated() > 0) {
        // '30' xname-attribute xname-string
        out_xname_off = large_ftell(out_fp);
        unsigned int sz =  out_xname_tab->allocated();
        t_ary ta(sz);
        SymTabGen gen(out_xname_tab, false);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            unsigned long ix = (unsigned long)h->stData;
            ta.ary[ix] = h->stTag;
        }
        if (!begin_compression(0))
            return (false);
        for (unsigned int i = 0; i < sz; i++) {
            if (!ta.ary[i])
                // uh-oh
                return (false);
            if (!write_unsigned(30))
                return (false);
            if (!write_unsigned(0))
                // xname-attribute
                return (false);
            if (!write_a_string(ta.ary[i]))
                return (false);
        }
        if (!end_compression())
            return (false);
    }
    return (true);
}


// Record the current file position and open the compressor.
//
bool
oas_out::begin_compression(const char *cellname)
{
    if (cellname && out_cgd) {
        if (out_lmux) {
            Errs()->add_error(
                "begin_compression: internal, already compressing!");
            return (false);
        }
        out_lmux = new cgd_layer_mux(cellname, out_cgd);
        return (true);
    }

    if (out_mode != Physical || out_use_compression == OAScompNone)
        return (true);
    if (out_compressing) {
        Errs()->add_error(
            "begin_compression: internal, already compressing!");
        return (false);
    }
    out_comp_start = large_ftell(out_fp);
    out_byte_count = out_comp_start;  // should already be equal
    out_compressing = true;
    return (true);
}


// End the compression - write the CBLOCK record, and set the file
// position to EOF.
//
bool
oas_out::end_compression()
{
    if (out_cgd) {
        if (!out_lmux)
            return (true);
        out_lmux->finalize();
        if (!out_lmux->grab_results())
            return (false);
        delete out_lmux;
        out_lmux = 0;
        out_modal = &out_default_modal;
        return (true);
    }

    if (!out_compressing)
        return (true);
    out_compressing = false;
    if (!out_zfile) {
        // Compressor was never created, so don't use a CBLOCK.
        int nbytes = out_byte_count - out_comp_start;
        if (nbytes > CGD_COMPR_BUFSIZE) {
            // "can't happen"
            Errs()->add_error(
                "end_compression: buffer overflow (internal error).");
            return (false);
        }
        for (int i = 0; i < nbytes; i++) {
            if (!write_char(out_compr_buf[i]))
                return (false);
        }
        return (true);
    }

    delete out_zfile;
    out_zfile = 0;

    uint64_t n_comp = large_ftell(out_tmp_fp);
    uint64_t n_ucomp = out_byte_count - out_comp_start;
    out_byte_count = out_comp_start;

    // CBLOCK: '34' '0' u-count c-count bytes
    if (!write_char(34))
        return (false);
    if (!write_char(0))
        return (false);
    if (!write_unsigned64(n_ucomp))
        return (false);
    if (!write_unsigned64(n_comp))
        return (false);

    rewind(out_tmp_fp);
    while (n_comp--) {
        if (!write_char(getc(out_tmp_fp)))
            return (false);
    }
    return (true);
}


// If using repetitions, the objects are dumped here.
//
bool
oas_out::flush_cache()
{
    if (out_cache)
        return (out_cache->flush());
    return (true);
}


bool
oas_out::write_repetition()
{
    if (out_repetition.type == 1) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        if (!write_unsigned(out_repetition.ydim))
            return (false);
        if (!write_unsigned(out_repetition.xspace))
            return (false);
        if (!write_unsigned(out_repetition.yspace))
            return (false);
    }
    else if (out_repetition.type == 2) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        if (!write_unsigned(out_repetition.xspace))
            return (false);
    }
    else if (out_repetition.type == 3) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.ydim))
            return (false);
        if (!write_unsigned(out_repetition.yspace))
            return (false);
    }
    else if (out_repetition.type == 4) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        int *a = out_repetition.array;
        for (unsigned int i = 0; i <= out_repetition.xdim; i++) {
            if (!write_unsigned(a[i]))
                return (false);
        }
    }
    else if (out_repetition.type == 5) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        if (!write_unsigned(out_repetition.ydim))
            return (false);
        int *a = out_repetition.array;
        for (unsigned int i = 0; i <= out_repetition.xdim; i++) {
            if (!write_unsigned(a[i]))
                return (false);
        }
    }
    else if (out_repetition.type == 6) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.ydim))
            return (false);
        int *a = out_repetition.array;
        for (unsigned int i = 0; i <= out_repetition.ydim; i++) {
            if (!write_unsigned(a[i]))
                return (false);
        }
    }
    else if (out_repetition.type == 7) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.ydim))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        int *a = out_repetition.array;
        for (unsigned int i = 0; i <= out_repetition.ydim; i++) {
            if (!write_unsigned(a[i]))
                return (false);
        }
    }
    else if (out_repetition.type == 8) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        if (!write_unsigned(out_repetition.ydim))
            return (false);
        if (!write_g_delta(out_repetition.array[0], out_repetition.array[1]))
            return (false);
        if (!write_g_delta(out_repetition.array[2], out_repetition.array[3]))
            return (false);
    }
    else if (out_repetition.type == 9) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        if (!write_g_delta(out_repetition.xspace, out_repetition.yspace))
            return (false);
    }
    else if (out_repetition.type == 10) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        int *a = out_repetition.array;
        for (unsigned int i = 0; i <= out_repetition.xdim; i++) {
            if (!write_g_delta(a[0], a[1]))
                return (false);
            a += 2;
        }
    }
    else if (out_repetition.type == 11) {
        if (!write_unsigned(out_repetition.type))
            return (false);
        if (!write_unsigned(out_repetition.xdim))
            return (false);
        if (!write_unsigned(out_repetition.ydim))
            return (false);
        int *a = out_repetition.array;
        for (unsigned int i = 0; i <= out_repetition.xdim; i++) {
            if (!write_g_delta(a[0], a[1]))
                return (false);
            a += 2;
        }
    }
    else
        return (false);
    return (true);
}


// Queue the property list passed (list is copied).
//
bool
oas_out::setup_properties(CDp *plist)
{
    for (CDp *pd = plist; pd; pd = pd->next_prp()) {
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
    return (true);
}

void
oas_out::set_layer_dt(int l, int d, int *pl, int *pd)
{
    if (pl)
        *pl = out_layer.layer;
    out_layer.layer = l;
    if (pd)
        *pd = out_layer.datatype;
    out_layer.datatype = d;
    if (out_lmux)
        out_modal = out_lmux->set_layer(l, d);
}

