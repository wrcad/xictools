
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
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_celldb.h"
#include "cd_chkintr.h"
#include "texttf.h"
#include <ctype.h>


// Convert the hierarchies under the named symbols to GDSII format
// according to the parameters.  The scale is used for the physical
// data.
//
// Return values:
//  OIok        normal return
//  OIerror     unspecified error (message in Errs)
//  OIabort     user interrupt
//
OItype
cFIO::ToGDSII(const stringlist *namelist, const FIOcvtPrms *prms)
{
    if (!namelist) {
        Errs()->add_error("Error in ToGDSII: null symbol list.");
        return (OIerror);
    }
    if (!prms) {
        Errs()->add_error("Error in ToGDSII: null params pointer.");
        return (OIerror);
    }

    const char *gds_fname = prms->destination();
    if (!gds_fname || !*gds_fname) {
        Errs()->add_error("Error in ToGDSII: null stream file name.");
        return (OIerror);
    }

    if (!namelist->next && namelist->string &&
            !strcmp(namelist->string, FIO_CUR_SYMTAB)) {
        // Secret code to dump the entire current symbol table.

        gds_out *gds = new gds_out;
        GCobject<gds_out*> gc_gds(gds);
        if (!gds->set_destination(gds_fname))
            return (OIerror);
        gds->assign_alias(
            NewWritingAlias(prms->alias_mask() | CVAL_GDS, false));
        gds->read_alias(gds_fname);

        bool ok = gds->write_all(Physical, prms->scale());
        if (ok && !fioStripForExport)
            ok = gds->write_all(Electrical, prms->scale());
        return (ok ? OIok : gds->was_interrupted() ? OIaborted : OIerror);
    }

    for (const stringlist *sl = namelist; sl; sl = sl->next) {
        if (!CDcdb()->findSymbol(sl->string)) {
            Errs()->add_error("Error in ToGDSII: %s not in symbol table.",
                sl->string ? sl->string : "(null)");
            return (OIerror);
        }
        if (prms->flatten())
            break;
    }

    gds_out *gds = new gds_out;
    GCobject<gds_out*> gc_gds(gds);
    if (!gds->set_destination(gds_fname))
        return (OIerror);
    gds->assign_alias(NewWritingAlias(prms->alias_mask() | CVAL_GDS, false));
    gds->read_alias(gds_fname);

    const BBox *AOI = prms->use_window() ? prms->window() : 0;
    bool ok;
    if (prms->flatten())
        // Just one name allowed, physical mode only.
        ok = gds->write_flat(namelist->string, prms->scale(), AOI,
            prms->clip());
    else {
        ok = gds->write(namelist, Physical, prms->scale());
        if (ok && !fioStripForExport)
            ok = gds->write(namelist, Electrical, prms->scale());
    }
    return (ok ? OIok : gds->was_interrupted() ? OIaborted : OIerror);
}
// End of cCD functions


gds_out::gds_out()
{
    out_filetype = Fgds;
    out_fp = 0;
    out_m_unit = 1.0;

    if (FIO()->GdsOutLevel() == 0) {
        out_max_poly = 8000;
        out_max_path = 8000;
        out_version = 7;
    }
    else if (FIO()->GdsOutLevel() == 1) {
        out_max_poly = 600;
        out_max_path = 200;
        out_version = 3;
    }
    else {
        out_max_poly = 200;
        out_max_path = 200;
        out_version = 3;
    }

    out_undef_count = 0;
    time_t tloc = time(0);
    out_date = *gmtime(&tloc);
    out_xy = new Point[mmMax(out_max_poly, out_max_path)];
    out_layer_oor_tab = 0;
    out_layer_cache = 0;

    out_bufcnt = 0;

    out_generation = 3;
    strcpy(out_libname, GDS_CD_LIBNAME);
    *out_lib1 = 0;
    *out_lib2 = 0;
    *out_font0 = 0;
    *out_font1 = 0;
    *out_font2 = 0;
    *out_font3 = 0;
    *out_attr = 0;
}


gds_out::~gds_out()
{
    delete out_fp;
    delete [] out_xy;
    delete out_layer_oor_tab;
    if (out_layer_cache) {
        out_layer_cache->clear();
        delete out_layer_cache;
    }
    clear_property_queue();
}


bool
gds_out::check_for_interrupt()
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


// The top cell of the hierarchy might contain header info for GDSII,
// but it is ok to pass null and ignore this.
//
bool
gds_out::write_header(const CDs *top_sdesc)
{
    strcpy(out_libname, GDS_CD_LIBNAME);
    if (top_sdesc) {
        for (CDp *pd = top_sdesc->prptyList(); pd; pd = pd->next_prp()) {

            switch (pd->value() - GDSII_PROPERTY_BASE) {
            case II_HEADER:
            case II_LIBNAME:
                break;
            case II_REFLIBS:
                sscanf(pd->string(), "%s %s", out_lib1, out_lib2);
                break;
            case II_FONTS:
                sscanf(pd->string(), "%s %s %s %s",
                    out_font0, out_font1, out_font2, out_font3);
                break;
            case II_GENERATIONS:
                sscanf(pd->string(), "%d", &out_generation);
                break;
            case II_ATTRTABLE:
                sscanf(pd->string(), "%s", out_attr);
                break;
            }
        }
    }
    if (out_mode == Physical) {
        if (!write_library(out_version, 1e-6/CDphysResolution,
                1.0/CDphysResolution, &out_date, &out_date, 0))
            return (false);
    }
    else {
        if (!write_library(out_version, 1e-6/CDelecResolution,
                1.0/CDelecResolution, &out_date, &out_date, 0))
            return (false);
    }

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
gds_out::write_object(const CDo *odesc, cvLchk *lchk)
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
gds_out::set_destination(const char *destination)
{
    out_fp = sFilePtr::newFilePtr(destination, "w");
    if (!out_fp) {
        Errs()->sys_error("open");
        Errs()->add_error("Unable to open %s.", destination);
        return (false);
    }
    out_filename = lstring::copy(destination);
    return (true);
}


bool
gds_out::queue_property(int val, const char *string)
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
gds_out::open_library(DisplayMode mode, double sc)
{
    out_mode = mode;
    out_scale = dfix(sc);
    if (mode == Physical)
        out_phys_scale = out_scale;
    out_needs_mult = (out_scale != 1.0);

    clear_property_queue();
    out_in_struct = false;
    out_symnum = 0;
    if (out_visited)
        out_visited->clear();
    out_layer.set_null();

    if (out_mode == Physical) {
        if (!write_library(out_version, 1e-6/CDphysResolution,
                1.0/CDphysResolution, &out_date, &out_date, 0))
            return (false);
    }
    else {
        if (!write_library(out_version, 1e-6/CDelecResolution,
                1.0/CDelecResolution, &out_date, &out_date, 0))
            return (false);
    }
    return (true);
}


bool
gds_out::write_library(int version, double munit, double uunit, tm *mdate,
    tm *adate, const char *libname)
{
    if (out_mode == Physical) {
        out_m_unit = FIO()->GdsMunit();
        if (out_m_unit >= 0.01 && out_m_unit <= 100.0) {
            if (out_m_unit != 1.0) {
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "Using M-UNITS=%.5e  U-UNITS=%.5e",
                    out_m_unit*1e-6/CDphysResolution,
                    out_m_unit/CDphysResolution);
            }
        }
        else {
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "Bad or out of range GdsMunit specified, ignored.");
            out_m_unit = 1.0;
        }
        munit *= out_m_unit;
        uunit *= out_m_unit;
        out_scale /= out_m_unit;
        out_phys_scale /= out_m_unit;
        out_needs_mult = (out_scale != 1.0);
    }

    begin_record(6, II_HEADER, 2);
    short_copy(version);
    begin_record(28, II_BGNLIB, 2);
    date_copy(mdate);
    date_copy(adate);
    if (libname)
        strcpy(out_libname, libname);
    if (!write_ascii_rec(out_libname, II_LIBNAME))
        return (false);

    if (out_lib1[0] || out_lib2[0]) {
        begin_record(92, II_REFLIBS, 6);
        name_copy(out_lib1);
        name_copy(out_lib2);
    }
    if (out_font0[0] || out_font1[0] || out_font2[0] || out_font3[0]) {
        begin_record(180, II_FONTS, 6);
        name_copy(out_font0);
        name_copy(out_font1);
        name_copy(out_font2);
        name_copy(out_font3);
    }
    if (out_attr[0]) {
        if (!write_ascii_rec(out_attr, II_ATTRTABLE))
            return (false);
    }
    begin_record(6, II_GENERATIONS, 2);
    short_copy(out_generation);
    begin_record(20, II_UNITS, 5);
    dbl_copy(uunit);
    dbl_copy(munit);
    return (flush_buf());
}


namespace {
    void struct_err(const char *what, const char *name)
    {
        Errs()->add_error("Error writing %s in %s.", what, name);
    }
}


bool
gds_out::write_struct(const char *name, tm *cdate, tm *mdate)
{
    if (out_no_struct) {
        out_in_struct = true;
        return (true);
    }
    if (out_in_struct) {
        if (!write_end_struct()) {
            struct_err("previous end record", name);
            return (false);
        }
    }
    out_in_struct = true;

    name = alias(name);
    out_cellBB = CDnullBB;
    out_layer.set_null();

    // write electrical cell properties
    if (out_mode == Electrical) {
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
            CDp *px = pd->dup();
            if (px) {
                px->scale(out_scale, out_phys_scale, out_mode);
                bool ret = write_property_rec(px->value(), px->string());
                delete px;
                if (!ret) {
                    struct_err("elec cell property", name);
                    return (false);
                }
            }
        }
    }

    begin_record(28, II_BGNSTR, 2);
    date_copy(cdate);
    date_copy(mdate);
    if (!write_ascii_rec(name, II_STRNAME)) {
        struct_err("name record", name);
        return (false);
    }
    out_struct_count++;

    // We can't use the extension above for physical mode properties,
    // since that would seriously break portability.

    if (out_mode == Physical && !FIO()->IsStripForExport()) {
        // Don't include gds header properties.
        int cnt = 0;
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
            if (prpty_gdsii(pd->value()))
                continue;
            cnt++;
        }
        if (cnt) {
#ifdef GDS_USE_NODE_PRPS
            // Create a SNAPNODE with a PLEX, and apply the cell
            // properties to this.  The reader will ignore the node
            // and apply the properties to the cell.

            begin_record(4, II_SNAPNODE, 0);

            begin_record(8, II_PLEX, 3);
            long_copy(XIC_NODE_PLEXNUM);

            begin_record(6, II_LAYER, 2);
            short_copy(0);
            begin_record(6, II_NODETYPE, 2);
            short_copy(0);
            begin_record(12, II_XY, 3);
            long_copy(0);
            long_copy(0);
            if (!flush_buf()) {
                struct_err("cell properties snapnode/plex", name);
                return (false);
            }
#else
            // Create a dummy label and apply the properties to the
            // label.  The reader will ignore the label and apply the
            // properties to the cell.  This is no longer done, as it
            // is bad to add something to the database that can alter
            // bounding boxes, etc.

            begin_record(4, II_TEXT, 0);
            begin_record(6, II_LAYER, 2);
            short_copy(0);
            begin_record(6, II_TEXTTYPE, 2);
            short_copy(0);
            begin_record(12, II_XY, 3);
            longcopy(0);
            longcopy(0);
            if (!write_ascii_rec(GDS_CELL_PROPERTY_MAGIC, II_STRING)) {
                struct_err("cell properties label", name);
                return (false);
            }
#endif

            for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
                if (prpty_gdsii(pd->value()))
                    continue;
                CDp *px = pd->dup();
                if (px) {
                    px->scale(out_scale, out_phys_scale, out_mode);
                    bool ret = write_property_rec(px->value(), px->string());
                    delete px;
                    if (!ret) {
                        struct_err("phys cell property", name);
                        return (false);
                    }
                }
            }
            begin_record(4, II_ENDEL, 0);
        }
    }

    return (flush_buf());
}


bool
gds_out::write_end_struct(bool force)
{
    if (out_in_struct || force) {
        if (out_no_struct)
            return (true);
        out_in_struct = false;
        begin_record(4, II_ENDSTR, 0);
        return (flush_buf());
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
gds_out::queue_layer(const Layer *layer, bool *check_mapping)
{
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
                // The LPP name is a hex number "LLDD" or  "LLLLDDDD".
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

    out_layer.layer = l;
    out_layer.datatype = d;

    if (l >= GDS_MAX_SPEC_LAYERS || d >= GDS_MAX_SPEC_DTYPES) {
        // Keep track of out of range layers/datatypes written,
        // issue a warning.
        unsigned long ll = (l << 16) | d;
        if (!out_layer_oor_tab)
            out_layer_oor_tab = new SymTab(false, false);
        if (SymTab::get(out_layer_oor_tab, ll) == ST_NIL) {
            out_layer_oor_tab->add(ll, 0, false);
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "Using non-standard layer %d or datatype %d, "
                "file may not be portable!", l, d);
        }
    }
    return (true);
}


bool
gds_out::write_box(const BBox *BB)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);
    BB->to_path(out_xy);

    begin_record(4, II_BOUNDARY, 0);
    begin_record(6, II_LAYER, 2);
    short_copy(out_layer.layer);
    begin_record(6, II_DATATYPE, 2);
    short_copy(out_layer.datatype);
    begin_record(8 * 5 + 4, II_XY, 3);
    for (int i = 0; i < 5; i++) {
        long_copy(scale(out_xy[i].x));
        long_copy(scale(out_xy[i].y));
    }
    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        if (FIO()->IsStripForExport() &&
                (pd->value() < 1 || pd->value() >= 128))
            continue;
        if (!write_property_rec(pd->value(), pd->string()))
            return (false);
    }
    begin_record(4, II_ENDEL, 0);
    return (flush_buf());
}


// If too many vertices, Break a polygon across the middle of its BB
// horizontally and write out the resulting clipped polygons.
//
bool
gds_out::write_poly(const Poly *po)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);
    if (po->numpts <= out_max_poly) {
        Point *p = po->points;
        int cnt;
        for (cnt = 0; cnt < po->numpts; cnt++)
            out_xy[cnt].set(scale(p[cnt].x), scale(p[cnt].y));
        if (cnt == 3) {
            // close the triangle
            out_xy[cnt] = out_xy[0];
            cnt++;
        }
        if (cnt >= 4) {
            begin_record(4, II_BOUNDARY, 0);
            begin_record(6, II_LAYER, 2);
            short_copy(out_layer.layer);
            begin_record(6, II_DATATYPE, 2);
            short_copy(out_layer.datatype);
            begin_record(8*cnt + 4, II_XY, 3);
            int end = GDS_BUFSIZ - 12;  // leave room for II_ENDEL
            for (int i = 0; i < cnt; i++) {
                if (out_bufcnt >= end) {
                    if (!flush_buf())
                        return (false);
                }
                long_copy(out_xy[i].x);
                long_copy(out_xy[i].y);
            }
            for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
                if (FIO()->IsStripForExport() &&
                        (pd->value() < 1 || pd->value() >= 128))
                    continue;
                if (!write_property_rec(pd->value(), pd->string()))
                    return (false);
            }
            begin_record(4, II_ENDEL, 0);
            return (flush_buf());
        }
        FIO()->ifPrintCvLog(IFLOG_WARN,
            "Ignoring polygon with too few vertices.");
        return (true);
    }
    FIO()->ifPrintCvLog(IFLOG_WARN,
        "Breaking polygon with too many vertices.");
    PolyList *p0 = po->divide(out_max_poly);
    for (PolyList *p = p0; p; p = p->next) {
        if (!write_poly(&p->po)) {
            PolyList::destroy(p0);
            return (false);
        }
    }
    PolyList::destroy(p0);
    return (true);
}


bool
gds_out::write_wire(const Wire *w)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);
    Wire wire(scale(w->wire_width()), w->wire_style(), w->numpts, out_xy);

    Point *p = w->points;
    int cnt = 0;
    if (wire.numpts == 1) {
        // need at least 2 coordinates
        out_xy[0].set(scale(p->x), scale(p->y));
        out_xy[1].set(scale(p->x), scale(p->y));
        wire.numpts = 2;
    }
    else {
        int numpts = wire.numpts;
        for ( ; numpts; numpts--) {
            out_xy[cnt].set(scale(p->x), scale(p->y));
            cnt++;
            if (cnt == out_max_path && numpts > 1) {
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "Breaking wire with too many vertices.");
                wire.numpts = cnt;
                if (!write_path(wire.wire_style(), wire.wire_width(),
                        wire.numpts))
                    return (false);
                cnt = 0;
                out_xy[0].set(scale(p->x), scale(p->y));
                cnt++;
            }
            p++;
        }
        wire.numpts = cnt;
    }
    if (wire.numpts) {
        if (!write_path(wire.wire_style(), wire.wire_width(), wire.numpts))
            return (false);
    }
    return (true);
}


bool
gds_out::write_text(const Text *ctext)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);

    if (!ctext->text || !*ctext->text)
        return (true);

    Text text(*ctext);
    text.setup_gds(out_mode);
    double magn = text.magn;
    int ptype = text.ptype;
    int pwidth = text.pwidth;

    // Look at property list for a II_TEXT text definition which
    // was added when the GDSII file was read.
    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        if ((pd->value() == II_TEXT + GDSII_PROPERTY_BASE) && pd->string()) {
            const char *string = pd->string();
            while (*string && *string != ';') {
                while (isspace(*string))
                    string++;
                const char *word1 = string;
                while (*string && !isspace(*string) && *string != ';')
                    string++;
                while (isspace(*string))
                    string++;
                if (!*string || *string == ';')
                    break;
                const char *word2 = string;
                while (*string && !isspace(*string) && *string != ';')
                    string++;

                if (!strncmp(word1, "WIDTH", 5))
                    sscanf(word2, "%d", &pwidth);
                else if (!strncmp(word1, "PTYPE", 5))
                    sscanf(word2, "%d", &ptype);
                // Always override these, the values in the Text struct
                // are assumed correct.
                // else if (!strncmp(word1, "MAG", 3))
                //     sscanf(word2, "%lf", &magn);
                // else if (!strncmp(s1, "ANGLE", 5))
                //     sscanf(s2, "%lf", &text.angle);
            }
            break;
        }
    }
    magn *= out_scale;
    pwidth = scale(pwidth);

    begin_record(4, II_TEXT, 0);
    begin_record(6, II_LAYER, 2);
    short_copy(out_layer.layer);
    begin_record(6, II_TEXTTYPE, 2);
    short_copy(out_layer.datatype);
    begin_record(6, II_PRESENTATION, 1);

    short_copy(text.hj + (text.vj << 2) + (text.font << 4));

    begin_record(6, II_PATHTYPE, 2);
    short_copy(ptype);
    begin_record(8, II_WIDTH, 3);
    long_copy(pwidth);

    int tf = 0;
    if (text.reflection)
        tf |= 0100000;
    if (text.abs_mag)
        tf |= 04;
    if (text.abs_ang)
        tf |= 02;
    begin_record(6, II_STRANS, 1);
    short_copy(tf);
    if (magn != 1.0) {
        begin_record(12, II_MAG, 5);
        dbl_copy(magn);
    }
    if (text.angle != 0) {
        begin_record(12, II_ANGLE, 5);
        dbl_copy(text.angle);
    }

    begin_record(12, II_XY, 3);
    long_copy(scale(text.x));
    long_copy(scale(text.y));

    if (out_mode == Physical) {
        unsigned int maxlen = out_version < 7 ? GDS_MAX_TEXT_LEN : 64532;
        if (strlen(text.text) >= maxlen) {
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "Label text too long, truncated to %d chars.", maxlen);
            char *string = lstring::copy(text.text);
            string[maxlen] = 0;
            bool ret = write_ascii_rec(string, II_STRING);
            delete [] string;
            if (!ret)
                return (false);
        }
        else if (!write_ascii_rec(text.text, II_STRING))
            return (false);
    }
    else {
        if (out_needs_mult) {
            char *string = lstring::copy(text.text);
            string = hyList::hy_scale(string, out_scale);
            bool ret = write_ascii_rec(string, II_STRING);
            delete [] string;
            if (!ret)
                return (false);
        }
        else if (!write_ascii_rec(text.text, II_STRING))
            return (false);
    }

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        if (FIO()->IsStripForExport() &&
                (pd->value() < 1 || pd->value() >= 128))
            continue;
        if (pd->value() == II_TEXT + GDSII_PROPERTY_BASE)
            continue;
        if (!write_property_rec(pd->value(), pd->string()))
            return (false);
    }
    // Add a property to store label size, etc.
    if (!FIO()->IsStripForExport() && text.width > 0 && text.height > 0) {
        char tbuf[80];
        sprintf(tbuf, "width %d height %d", scale(text.width),
            scale(text.height));
        // Add electrical property display flags.
        if (out_mode == Electrical) {
            if (text.xform & TXTF_SHOW)
                strcat(tbuf, " show");
            else if (text.xform & TXTF_HIDE)
                strcat(tbuf, " hide");
            if (text.xform & TXTF_TLEV)
                strcat(tbuf, " tlev");
            if (text.xform & TXTF_LIML)
                strcat(tbuf, " liml");
        }
        if (!write_property_rec(XICP_GDS_LABEL_SIZE, tbuf))
            return (false);
    }
    begin_record(4, II_ENDEL, 0);
    return (flush_buf());
}


bool
gds_out::write_sref(const Instance *inst)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);

    const char *cellname = alias(inst->name);
    if (inst->nx > 1 || inst->ny > 1) {
        // create an AREF

        begin_record(4, II_AREF, 0);
        if (!write_ascii_rec(cellname, II_SNAME))
            return (false);

        int tf = 0;
        if (inst->reflection)
            tf |= 0100000;
        if (inst->abs_mag)
            tf |= 04;
        if (inst->abs_ang)
            tf |= 02;
        begin_record(6, II_STRANS, 1);
        short_copy(tf);
        if (inst->magn != 1.0) {
            begin_record(12, II_MAG, 5);
            dbl_copy(inst->magn);
        }
        if (inst->angle != 0) {
            begin_record(12, II_ANGLE, 5);
            dbl_copy(inst->angle);
        }

        begin_record(8, II_COLROW, 2);
        short_copy(inst->nx);
        short_copy(inst->ny);
        begin_record(28, II_XY, 3);
        long_copy(scale(inst->origin.x));
        long_copy(scale(inst->origin.y));
        Point p[2];
        if (!inst->get_array_pts(p))
            return (false);
        long_copy(scale(p[0].x));
        long_copy(scale(p[0].y));
        long_copy(scale(p[1].x));
        long_copy(scale(p[1].y));
    }
    else {
        // create an SREF

        begin_record(4, II_SREF, 0);
        if (!write_ascii_rec(cellname, II_SNAME))
            return (false);

        int tf = 0;
        if (inst->reflection)
            tf |= 0100000;
        if (inst->abs_mag)
            tf |= 04;
        if (inst->abs_ang)
            tf |= 02;
        begin_record(6, II_STRANS, 1);
        short_copy(tf);
        if (inst->magn != 1.0) {
            begin_record(12, II_MAG, 5);
            dbl_copy(inst->magn);
        }
        if (inst->angle != 0) {
            begin_record(12, II_ANGLE, 5);
            dbl_copy(inst->angle);
        }

        begin_record(12, II_XY, 3);
        long_copy(scale(inst->origin.x));
        long_copy(scale(inst->origin.y));
    }
    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        if (FIO()->IsStripForExport() &&
                (pd->value() < 1 || pd->value() >= 128))
            continue;
        CDp *px = pd->dup();
        if (px) {
            px->scale(out_scale, out_phys_scale, out_mode);
            bool ret = write_property_rec(px->value(), px->string());
            delete px;
            if (!ret)
                return (false);
        }
    }
    begin_record(4, II_ENDEL, 0);
    return (flush_buf());
}


bool
gds_out::write_endlib(const char*)
{
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    begin_record(4, II_ENDLIB, 0);
    if (!flush_buf())
        return (false);

    // The out_byte_count should now be accurate in Physical mode.  In
    // Electrical mode, out_byte_count might have been reset to 0 at the
    // start, so is different than the file offset.  Reset it.
    out_byte_count = out_fp->z_tell();

    // pad with nulls
    int i = GDS_PHYS_REC_SIZE - (out_byte_count % GDS_PHYS_REC_SIZE);
    if (i == GDS_PHYS_REC_SIZE)
        i = 0;

    if (out_mode == Physical)
        out_byte_count += i;

    while (i > 0) {
        while (i && out_bufcnt < GDS_BUFSIZ) {
            out_buffer[out_bufcnt++] = 0;
            i--;
        }
        if (!flush_buf())
            return (false);
    }
    return (true);
}


bool
gds_out::write_info(Attribute*, const char*)
{
    return (true);
}
// End of virtual function overrides


// Copy a date record to the output buffer.
//
void
gds_out::date_copy(struct tm *datep)
{
    short_copy(datep->tm_year);
    short_copy(datep->tm_mon + 1);
    short_copy(datep->tm_mday);
    short_copy(datep->tm_hour);
    short_copy(datep->tm_min);
    short_copy(datep->tm_sec);
}


// Function to transfer a double precision number to a stream file.
// The first character output will contain the exponent field.  The
// last character output will contain the least significant byte of
// the mantissa field.
//
// This function is inefficient, but there are typically few
// conversions required and portability is an issue.
//
void
gds_out::dbl_copy(double r)
{
    unsigned char *b = out_buffer + out_bufcnt;
    out_bufcnt += 8;
    int i, sign = 0;
    if (r == 0.0) {
        for (i = 0; i < 8; i++)
            b[i] = 0;
        return;
    }
    if (r < 0.0) {
        sign = 1;
        r = -r;
    }

    // normalize to 1/16 < r <= 1
    i = 0;
    int exp;
    if (r >= 1.0) {
        while (r >= 1.0) {
            r /= 16.0;
            i++;
        }
        if (i > 63) {
            // overflow
            for (i = 0; i < 8; i++)
                b[i] = 0xff;
            if (!sign)
                b[0] &= 0x7f;
            return;
        }
        exp = i + 64;
    }
    else if (r < 1/16.0) {
        while (r < 1/16.0 && i < 64) {
            r *= 16.0;
            i++;
        }
        if (i > 63) {
            // underflow
            for (i = 0; i < 8; i++)
                b[i] = 0;
            return;
        }
        exp = 64 - i;
    }
    else
        exp = 64;
    for (i = 1; i <= 7; i++) {
        r *= 256.0;
        b[i] = (unsigned char)r;
        r -= b[i];
    }
    b[0] = exp;
    if (sign)
        b[0] |= 0x80;
}


// Flush the output buffer to disk.
//
bool
gds_out::flush_buf()
{
    if (out_bufcnt > 0) {
        if (out_fp->z_write(out_buffer, 1, out_bufcnt) != out_bufcnt) {
            Errs()->sys_error("write");
            Errs()->add_error("write error, file system full?");
            return (false);
        }
    }
    out_bufcnt = 0;
    return (true);
}


bool
gds_out::write_property_rec(int val, const char *string)
{
    int end = GDS_BUFSIZ - 10;  // 6 here + 4 in write_ascii_rec
    if (out_bufcnt >= end) {
        if (!flush_buf())
            return (false);
    }

    begin_record(6, II_PROPATTR, 2);
    short_copy(val);
    if (!write_ascii_rec(string, II_PROPVALUE))
        return (false);
    return (true);
}


bool
gds_out::write_ascii_rec(const char *c, int type)
{
    int i = strlen(c);
    int addone = i & 1;
    int rsz = i + 4 + addone;
    if (rsz > 0x10000) {
        // String too long for a GDSII record.
        if (FIO()->GdsTruncateLongStrings()) {
            i = 0x10000 - 5;
            addone = 1;
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "String too long for GDSII record, truncated.");
        }
        else {
            Errs()->add_error("String too long for GDSII record.");
            return (false);
        }
    }
    begin_record(i + 4 + addone, type, 6);
    if (!flush_buf())
        return (false);

    if (out_fp->z_write(c, 1, i) == i && (!addone || out_fp->z_putc(0) != EOF))
        return (true);

    Errs()->sys_error("write");
    Errs()->add_error("write error, file system full?");
    return (false);
}


bool
gds_out::write_path(int style, int width, int numpts)
{
    begin_record(4, II_PATH, 0);
    begin_record(6, II_LAYER, 2);
    short_copy(out_layer.layer);
    begin_record(6, II_DATATYPE, 2);
    short_copy(out_layer.datatype);
    begin_record(6, II_PATHTYPE, 2);
    short_copy(style);
    begin_record(8, II_WIDTH, 3);
    long_copy(width);
    begin_record(8*numpts + 4, II_XY, 3);

    int end = GDS_BUFSIZ - 12;  // leave room for II_ENDEL
    for (int i = 0; i < numpts; i++) {
        if (out_bufcnt >= end) {
            if (!flush_buf())
                return (false);
        }
        long_copy(out_xy[i].x);
        long_copy(out_xy[i].y);
    }

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        if (FIO()->IsStripForExport() &&
                (pd->value() < 1 || pd->value() >= 128))
            continue;
        if (!write_property_rec(pd->value(), pd->string()))
            return (false);
    }
    begin_record(4, II_ENDEL, 0);
    return (flush_buf());
}


bool
gds_out::write_object_prv(const CDo *odesc)
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
        ret = text.set(&la, out_mode, Fgds);
        if (ret) {
            if (out_stk)
                text.transform(out_stk);
            ret = write_text(&text);
        }
        delete [] text.text;
    }
    return (ret);
}

