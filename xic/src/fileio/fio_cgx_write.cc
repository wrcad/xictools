
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
#include "fio_cgx.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_celldb.h"
#include "cd_chkintr.h"
#include "texttf.h"


// Have to break polys/wires larger than this
#define CGX_MAX_VERTS 8000

// Convert the hierarchies under the named symbols to CGX format
// according to the parameters.  The scale is used for the physical
// data.
//
// Return values:
//  OIok        normal return
//  OIerror     unspecified error (message in Errs)
//  OIabort     user interrupt
//
OItype
cFIO::ToCGX(const stringlist *namelist, const FIOcvtPrms *prms)
{
    if (!namelist) {
        Errs()->add_error("Error in ToCGX: null symbol list.");
        return (OIerror);
    }
    if (!prms) {
        Errs()->add_error("Error in ToCGX: null params pointer.");
        return (OIerror);
    }

    const char *cgx_fname = prms->destination();
    if (!cgx_fname || !*cgx_fname) {
        Errs()->add_error("Error in ToCGX: null cgx file name.");
        return (OIerror);
    }

    if (!namelist->next && namelist->string &&
            !strcmp(namelist->string, FIO_CUR_SYMTAB)) {
        // Secret code to dump the entire current symbol table.

        cgx_out *cgx = new cgx_out();
        GCobject<cgx_out*> gc_cgx(cgx);
        if (!cgx->set_destination(cgx_fname))
            return (OIerror);
        cgx->assign_alias(NewWritingAlias(prms->alias_mask(), false));
        cgx->read_alias(cgx_fname);

        bool ok = cgx->write_all(Physical, prms->scale());
        if (ok && !fioStripForExport)
            ok = cgx->write_all(Electrical, prms->scale());
        return (ok ? OIok : cgx->was_interrupted() ? OIaborted : OIerror);
    }

    for (const stringlist *sl = namelist; sl; sl = sl->next) {
        if (!CDcdb()->findSymbol(sl->string)) {
            Errs()->add_error("Error in ToCGX: %s not in symbol table.",
                sl->string ? sl->string : "(null)");
            return (OIerror);
        }
        if (prms->flatten())
            break;
    }

    cgx_out *cgx = new cgx_out();
    GCobject<cgx_out*> gc_cgx(cgx);
    if (!cgx->set_destination(cgx_fname))
        return (OIerror);
    cgx->assign_alias(NewWritingAlias(prms->alias_mask(), false));
    cgx->read_alias(cgx_fname);

    const BBox *AOI = prms->use_window() ? prms->window() : 0;
    bool ok;
    if (prms->flatten())
        // Just one name allowed, physical mode only.
        ok = cgx->write_flat(namelist->string, prms->scale(), AOI,
            prms->clip());
    else {
        ok = cgx->write(namelist, Physical, prms->scale());
        if (ok && !fioStripForExport)
            ok = cgx->write(namelist, Electrical, prms->scale());
    }
    return (ok ? OIok : cgx->was_interrupted() ? OIaborted : OIerror);
}
// End of cFIO functions.


cgx_out::cgx_out()
{
    out_filetype = Fcgx;
    out_layer_written = true;
    out_lname_temp = 0;
    out_bufcnt = 0;
    out_fp = 0;
    out_boxcnt = 0;
}


cgx_out::~cgx_out()
{
    delete [] out_lname_temp;
    delete out_fp;
}


bool
cgx_out::check_for_interrupt()
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
cgx_out::flush_cache()
{
    if (out_boxcnt) {
        if (!write_box(out_boxbuf))
            return (false);
        out_boxcnt = 0;
    }
    return (true);
}


bool
cgx_out::write_header(const CDs*)
{
    time_t tloc = time(0);
    tm now = *gmtime(&tloc);
    if (out_mode == Physical) {
        if (!write_library(0, 1e-6/CDphysResolution, 1.0/CDphysResolution,
                &now, &now, "xic-cgx-physical"))
            return (false);
    }
    else {
        if (!write_library(0, 1e-6/CDelecResolution, 1.0/CDelecResolution,
                &now, &now, "xic-cgx-electrical"))
            return (false);
    }
    return (true);
}


// Write an object, properties are already queued.
//
bool
cgx_out::write_object(const CDo *odesc, cvLchk *lchk)
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
        BBox BB(odesc->oBB());
        bool wrote_poly = false;
        if (out_stk) {
            Point *p;
            out_stk->TBB(&BB, &p);
            if (p) {
                Poly po(5, p);
                ret = write_poly(&po);
                delete [] p;
                wrote_poly = true;
            }
        }
        if (!wrote_poly) {
            if (odesc->prpty_list())
                ret = write_box(&BB);
            else {
                // Cache boxes without properties.  Probably should
                // cache boxes with identical properties, but this
                // would be a lot more work
                out_boxbuf[out_boxcnt++] = BB;
                if (out_boxcnt == CGX_BOX_MAX) {
                    ret = write_box(out_boxbuf);
                    out_boxcnt = 0;
                }
            }
        }
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
        ret = text.set(&la, out_mode, Fcgx);
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
cgx_out::set_destination(const char *destination)
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
cgx_out::queue_property(int val, const char *string)
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
cgx_out::open_library(DisplayMode mode, double sc)
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
    out_boxcnt = 0;

    time_t tloc = time(0);
    tm now = *gmtime(&tloc);
    if (out_mode == Physical) {
        if (!write_library(0, 1e-6/CDphysResolution, 1.0/CDphysResolution,
                &now, &now, "xic-cgx-physical"))
            return (false);
    }
    else {
        if (!write_library(0, 1e-6/CDelecResolution, 1.0/CDelecResolution,
                &now, &now, "xic-cgx-electrical"))
            return (false);
    }

    return (true);
}


bool
cgx_out::write_library(int vers, double munit, double uunit, tm *cdate,
    tm *mdate, const char *libname)
{
    int dsize = 36 + strlen(libname);
    if (dsize & 1)
        dsize++;
    out_buffer[out_bufcnt++] = 'c';
    out_buffer[out_bufcnt++] = 'g';
    out_buffer[out_bufcnt++] = 'x';
    out_buffer[out_bufcnt++] = 0;
    begin_record(dsize, R_LIBRARY, vers);
    dbl_copy(munit);
    dbl_copy(uunit);
    date_copy(cdate);
    date_copy(mdate);
    if (!write_ascii_rec(libname))
        return (false);
    return (true);
}


bool
cgx_out::write_struct(const char *name, tm *cdate, tm *mdate)
{
    if (out_no_struct) {
        out_in_struct = true;
        return (true);
    }
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    out_in_struct = true;

    out_cellBB = CDnullBB;
    out_layer.set_null();
    out_layer_written = true;

    name = alias(name);
    int dsize = 20 + strlen(name);
    if (dsize & 1)
        dsize++;
    begin_record(dsize, R_STRUCT, 0);
    date_copy(cdate);
    date_copy(mdate);
    if (!write_ascii_rec(name))
        return (false);
    out_struct_count++;

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        CDp *px = pd->dup();
        if (px) {
            px->scale(out_scale, out_phys_scale, out_mode);
            bool ret = write_cprpty_rec(px->value(), px->string());
            delete px;
            if (!ret)
                return (false);
        }
    }
    return (true);
}


bool
cgx_out::write_end_struct(bool)
{
    if (out_no_struct)
        return (true);
    out_in_struct = false;
    return (true);
}


bool
cgx_out::queue_layer(const Layer *layer, bool*)
{
    if (!out_in_struct)
        return (true);

    if (out_layer.layer != layer->layer ||
            out_layer.datatype != layer->datatype ||
            (layer->name && (!out_layer.name ||
                strcmp(out_layer.name, layer->name)))) {

        out_layer.layer = layer->layer;
        out_layer.datatype = layer->datatype;
        delete [] out_lname_temp;
        out_lname_temp = lstring::copy(layer->name);
        out_layer.name = out_lname_temp;
        out_layer_written = false;
    }
    return (true);
}


bool
cgx_out::write_box(const BBox *boxes)
{
    if (!check_for_interrupt())
        return (false);
    if (out_in_struct) {
        if (!write_layer_rec())
            return (false);
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
            if (!write_property_rec(pd->value(), pd->string()))
                return (false);
        }

        int numboxes = (boxes == out_boxbuf ? out_boxcnt : 1);
        int dsize = 4 + numboxes*16;
        begin_record(dsize, R_BOX, 0);
        int end = CGX_BUFSIZ - 16;
        while (numboxes--) {
            if (out_bufcnt >= end) {
                if (!flush_buf())
                    return (false);
            }
            long_copy(scale(boxes->left));
            long_copy(scale(boxes->bottom));
            long_copy(scale(boxes->right));
            long_copy(scale(boxes->top));
            boxes++;
        }
    }
    return (flush_buf());
}


bool
cgx_out::write_poly(const Poly *poly)
{
    if (!check_for_interrupt())
        return (false);
    if (out_in_struct) {
        if (!write_layer_rec())
            return (false);
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
            if (!write_property_rec(pd->value(), pd->string()))
                return (false);
        }
        if (poly->numpts > CGX_MAX_VERTS) {
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "Breaking polygon with more than %d vertices.", CGX_MAX_VERTS);
            PolyList *p0 = poly->divide(CGX_MAX_VERTS);
            for (PolyList *p = p0; p; p = p->next) {
                int dsize = 4 + p->po.numpts*8;
                begin_record(dsize, R_POLY, 0);
                int end = CGX_BUFSIZ - 8;
                for (int i = 0; i < p->po.numpts; i++) {
                    if (out_bufcnt >= end) {
                        if (!flush_buf()) {
                            PolyList::destroy(p0);
                            return (false);
                        }
                    }
                    long_copy(scale(p->po.points[i].x));
                    long_copy(scale(p->po.points[i].y));
                }
                if (!flush_buf())
                    return (false);
            }
            PolyList::destroy(p0);
        }
        else {
            int dsize = 4 + poly->numpts*8;
            begin_record(dsize, R_POLY, 0);
            int end = CGX_BUFSIZ - 8;
            for (int i = 0; i < poly->numpts; i++) {
                if (out_bufcnt >= end) {
                    if (!flush_buf())
                        return (false);
                }
                long_copy(scale(poly->points[i].x));
                long_copy(scale(poly->points[i].y));
            }
            if (!flush_buf())
                return (false);
        }
    }
    return (true);
}


bool
cgx_out::write_wire(const Wire *wire)
{
    if (!check_for_interrupt())
        return (false);
    if (out_in_struct) {
        if (!write_layer_rec())
            return (false);
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
            if (!write_property_rec(pd->value(), pd->string()))
                return (false);
        }
        if (wire->numpts > CGX_MAX_VERTS) {
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "Breaking wire with more than %d vertices.", CGX_MAX_VERTS);
            int si = 0;
            int ei = CGX_MAX_VERTS;
            int sz = CGX_MAX_VERTS;
            for (;;) {
                int dsize = 8 + sz*8;
                begin_record(dsize, R_WIRE, wire->wire_style());
                long_copy(scale(wire->wire_width()));
                int end = CGX_BUFSIZ - 8;
                for (int i = si; i < ei; i++) {
                    if (out_bufcnt >= end) {
                        if (!flush_buf())
                            return (false);
                    }
                    long_copy(scale(wire->points[i].x));
                    long_copy(scale(wire->points[i].y));
                }
                if (!flush_buf())
                    return (false);
                si = ei - 1;
                sz = wire->numpts - ei;
                if (sz <= 1)
                    break;
                if (sz > CGX_MAX_VERTS)
                    sz = CGX_MAX_VERTS;
                ei = si + sz;
            }
        }
        else {
            int dsize = 8 + wire->numpts*8;
            begin_record(dsize, R_WIRE, wire->wire_style());
            long_copy(scale(wire->wire_width()));
            int end = CGX_BUFSIZ - 8;
            for (int i = 0; i < wire->numpts; i++) {
                if (out_bufcnt >= end) {
                    if (!flush_buf())
                        return (false);
                }
                long_copy(scale(wire->points[i].x));
                long_copy(scale(wire->points[i].y));
            }
            if (!flush_buf())
                return (false);
        }
    }
    return (true);
}


namespace {
    inline int
    xicx2cgxx(int xicx)
    {
        if (xicx & TXTF_MX) {
            cTfmStack stk;
            stk.TSetTransformFromXform(xicx, 0, 0);
            CDtf tf;
            stk.TCurrent(&tf);
            stk.TPop();
            int mask = (TXTF_ROT | TXTF_MY | TXTF_MX);
            int xf = tf.get_xform();
            xicx &= ~mask;
            xicx |= xf & mask;
        }
        return ((xicx & 0xf) | ((xicx & 0x1e0) >> 1));
    }
}


namespace {
    inline void write_err(const char *name)
    {
        Errs()->sys_error(name);
        Errs()->add_error("write error, file system full?");
    }
}


bool
cgx_out::write_text(const Text *text)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);

    if (!text->text || !*text->text)
        return (true);

    if (!write_layer_rec())
        return (false);
    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        if (!write_property_rec(pd->value(), pd->string()))
            return (false);
    }

    // Hack for label flags.  To pass the flags, the string will be
    // followed by a null byte, followed by the flags byte, optionally
    // followed by another null byte to make the length even.
    //
    unsigned char flag_byte = 0;
    if (text->xform & TXTF_SHOW)
        flag_byte = CGX_LA_SHOW;
    else if (text->xform & TXTF_HIDE)
        flag_byte = CGX_LA_HIDE;
    if (text->xform & TXTF_TLEV)
        flag_byte = CGX_LA_TLEV;
    if (text->xform & TXTF_LIML)
        flag_byte = CGX_LA_LIML;
    char *string = lstring::copy(text->text);
    if (out_mode == Electrical && out_needs_mult)
        string = hyList::hy_scale(string, out_scale);

    int len = strlen(string);
    int dsize = 16 + len;
    if (flag_byte)
        dsize += 2;
    if (dsize & 1)
        dsize++;
    if (dsize >= 0x10000) {
        Errs()->add_error("String too long for record size.");
        return (false);
    }
    begin_record(dsize, R_TEXT, xicx2cgxx(text->xform));
    long_copy(scale(text->x));
    long_copy(scale(text->y));
    long_copy(scale(text->width));

    if (flag_byte) {
        if (!flush_buf()) {
            delete [] string;
            return (false);
        }
        if (out_fp->z_write(string, 1, len) != len ||
                out_fp->z_putc(0) == EOF) {
            delete [] string;
            write_err("write_text");
            return (false);
        }
        delete [] string;
        if (out_fp->z_putc(flag_byte) == EOF) {
            write_err("write_text");
            return (false);
        }
        if ((len & 1) && out_fp->z_putc(0) == EOF) {
            write_err("write_text");
            return (false);
        }
        return (true);
    }
    bool ret = write_ascii_rec(string);
    delete [] string;
    return (ret);
}


bool
cgx_out::write_sref(const Instance *inst)
{
    if (!check_for_interrupt())
        return (false);
    if (out_in_struct) {

        const char *cellname = alias(inst->name);
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
            CDp *px = pd->dup();
            if (px) {
                px->scale(out_scale, out_phys_scale, out_mode);
                bool ret = write_property_rec(px->value(), px->string());
                delete px;
                if (!ret)
                    return (false);
            }
        }

        int flags = 0;
        if (inst->reflection)
            flags |= RF_REFLECT;
        if (inst->magn != 1.0)
            flags |= RF_MAGN;
        if (inst->nx > 1 || inst->ny > 1)
            flags |= RF_ARRAY;
        if (inst->angle != 0.0)
            flags |= RF_ANGLE;
        int dsize = 4;
        if (flags & RF_ARRAY)
            dsize += 32;
        else
            dsize += 8;
        if (flags & RF_MAGN)
            dsize += 8;
        if (flags & RF_ANGLE)
            dsize += 8;
        dsize += strlen(cellname);
        if (dsize & 1)
            dsize++;
        begin_record(dsize, R_SREF, flags);
        long_copy(scale(inst->origin.x));
        long_copy(scale(inst->origin.y));
        if (flags & RF_ANGLE)
            dbl_copy(inst->angle);
        if (flags & RF_MAGN)
            dbl_copy(inst->magn);
        if (flags & RF_ARRAY) {
            long_copy(inst->nx);
            long_copy(inst->ny);
            Point p[2];
            if (!inst->get_array_pts(p))
                return (false);
            long_copy(scale(p[0].x));
            long_copy(scale(p[0].y));
            long_copy(scale(p[1].x));
            long_copy(scale(p[1].y));
        }
        if (!write_ascii_rec(cellname))
            return (false);
    }
    return (true);
}


bool
cgx_out::write_endlib(const char*)
{
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    begin_record(4, R_ENDLIB, 0);
    return (flush_buf());
}


bool
cgx_out::write_info(Attribute*, const char*)
{
    return (true);
}
// End of virtual overrides


void
cgx_out::date_copy(tm *datep)
{
    // 8 bytes, different from GDSII
    short_copy(datep->tm_year);
    out_buffer[out_bufcnt++] = datep->tm_mon + 1;
    out_buffer[out_bufcnt++] = datep->tm_mday;
    out_buffer[out_bufcnt++] = datep->tm_hour;
    out_buffer[out_bufcnt++] = datep->tm_min;
    out_buffer[out_bufcnt++] = datep->tm_sec;
    out_buffer[out_bufcnt++] = 0;
}


// Function to transfer a double precision number to a Stream file.
// The first character output will contain the exponent field.  The
// last character output will contain the least significant byte of
// the mantissa field.
//
// This function is inefficient, but there are typically few
// conversions required and portability is an issue.
//
void
cgx_out::dbl_copy(double r)
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
cgx_out::flush_buf()
{
    if (out_bufcnt > 0) {
        if (out_fp->z_write(out_buffer, 1, out_bufcnt) != out_bufcnt) {
            write_err("flush_buf");
            return (false);
        }
    }
    out_bufcnt = 0;
    return (true);
}


bool
cgx_out::write_layer_rec()
{
    if (!out_layer_written && out_in_struct) {
        out_layer_written = true;

        int dsize = 8 + strlen(out_layer.name);
        if (dsize & 1)
            dsize++;
        begin_record(dsize, R_LAYER, 0);

        short_copy(out_layer.layer);
        short_copy(out_layer.datatype);
        if (!write_ascii_rec(out_layer.name))
            return (false);
    }
    return (true);
}


bool
cgx_out::write_cprpty_rec(int val, const char *string)
{
    int dsize = 8 + strlen(string);
    if (dsize & 1)
        dsize++;
    if (dsize >= 0x10000) {
        Errs()->add_error("String too long for record size.");
        return (false);
    }
    begin_record(dsize, R_CPRPTY, 0);
    long_copy(val);
    if (!write_ascii_rec(string))
        return (false);
    return (true);
}


bool
cgx_out::write_property_rec(int val, const char *string)
{
    int dsize = 8 + strlen(string);
    if (dsize & 1)
        dsize++;
    if (dsize >= 0x10000) {
        Errs()->add_error("String too long for record size.");
        return (false);
    }
    begin_record(dsize, R_PROPERTY, 0);
    long_copy(val);
    if (!write_ascii_rec(string))
        return (false);
    return (true);
}


bool
cgx_out::write_ascii_rec(const char *c)
{
    if (!flush_buf())
        return (false);
    int i = strlen(c);
    bool addone = (i & 1);

    if (out_fp->z_write(c, 1, i) == i && (!addone || out_fp->z_putc(0) != EOF))
        return (true);

    write_err("write_ascii_rec");
    return (false);
}

