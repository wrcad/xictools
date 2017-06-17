
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
 $Id: fio_cif_write.cc,v 1.60 2016/02/21 18:49:42 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_cif.h"
#include "fio_cvt_base.h"
#include "fio_gencif.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_celldb.h"
#include "cd_chkintr.h"


// Map layer names to trditional 4-char names.  We don't do this
// ayymore.
//#define MAP_LNAMES

// Set the enum values for the code in vstring.  Return true if ok,
// false and pop up an error message if bad syntax.
//
bool
CIFstyle::set(const char *vstring)
{
    int cn, la, lb;
    if (!strcmp(vstring, "a")) {
        // Stanford
        cn = EXTcnameNCA;
        la = EXTlayerDef;
        lb = EXTlabelKIC;
    }
    else if (!strcmp(vstring, "b")) {
        // NCA
        cn = EXTcnameNCA;
        la = EXTlayerNCA;
        lb = EXTlabelNCA;
    }
    else if (!strcmp(vstring, "i")) {
        // Icarus
        cn = EXTcnameICARUS;
        la = EXTlayerDef;
        lb = EXTlabelKIC;
    }
    else if (!strcmp(vstring, "s")) {
        // Sif
        cn = EXTcnameSIF;
        la = EXTlayerDef;
        lb = EXTlabelKIC;
    }
    else if (!strcmp(vstring, "m")) {
        // Mextra
        cn = EXTcnameDef;
        la = EXTlayerDef;
        lb = EXTlabelMEXTRA;
    }
    else if (!strcmp(vstring, "n")) {
        // None
        cn = EXTcnameNone;
        la = EXTlayerDef;
        lb = EXTlabelNone;
    }
    else if (!strcmp(vstring, "x")) {
        // Xic
        cn = EXTcnameDef;
        la = EXTlayerDef;
        lb = EXTlabelDef;
    }
    else if (sscanf(vstring, "%d:%d:%d", &cn, &la, &lb) != 3) {
        Errs()->add_error("Incorrect CifOutStyle syntax.");
        return (false);
    }
    if (cn >= 0 && cn <= EXTcnameNone && la >= 0 && la <= EXTlayerNCA &&
            lb >= 0 && lb <= EXTlabelNone) {
        cs_cnametype = (EXTcnameType)cn;
        cs_layertype = (EXTlayerType)la;
        cs_labeltype = (EXTlabelType)lb;
    }
    else {
        Errs()->add_error("Incorrect CifOutStyle index.");
        return (false);
    }
    return (true);
}


// Set the default values.
//
void
CIFstyle::set_def()
{
    cs_cnametype = EXTcnameDef;
    cs_layertype = EXTlayerDef;
    cs_labeltype = EXTlabelDef;
}


// Return a string (should be freed) containing the code.
//
char *
CIFstyle::string()
{
    char buf[32];
    if (cs_cnametype == EXTcnameNCA && cs_layertype == EXTlayerDef &&
            cs_labeltype == EXTlabelKIC)
        // Stanford
        strcpy(buf, "a");
    else if (cs_cnametype == EXTcnameNCA && cs_layertype == EXTlayerNCA &&
            cs_labeltype == EXTlabelNCA)
        // NCA
        strcpy(buf, "b");
    else if (cs_cnametype == EXTcnameICARUS && cs_layertype == EXTlayerDef &&
            cs_labeltype == EXTlabelKIC)
        // Icarus
        strcpy(buf, "i");
    else if (cs_cnametype == EXTcnameSIF && cs_layertype == EXTlayerDef &&
            cs_labeltype == EXTlabelKIC)
        // Sif
        strcpy(buf, "s");
    else if (cs_cnametype == EXTcnameDef && cs_layertype == EXTlayerDef &&
            cs_labeltype == EXTlabelMEXTRA)
        // Mextra
        strcpy(buf, "m");
    else if (cs_cnametype == EXTcnameNone && cs_layertype == EXTlayerDef &&
            cs_labeltype == EXTlabelNone)
        // None
        strcpy(buf, "n");
    else if (cs_cnametype == EXTcnameDef && cs_layertype == EXTlayerDef &&
            cs_labeltype == EXTlabelDef)
        // Xic
        strcpy(buf, "x");
    else
        sprintf(buf, "%d:%d:%d", cs_cnametype, cs_layertype, cs_labeltype);
    return (lstring::copy(buf));
}
// End of CIFstyle functions


// Output a coordinate list, keeping lines less than 80 chars.
//
void
sCifGen::out_path(FILE *fp, const Point *points, int numpts, char *buf,
    int x, int y)
{
    int len = strlen(buf);
    for (const Point *pair = points; numpts; pair++, numpts--) {
        char buf1[80];
        sprintf(buf1, " %d %d", pair->x - x, pair->y - y);
        int len1 = strlen(buf1);
        if (len + len1 < 79) {
            strcat(buf, buf1);
            len += len1;
        }
        else {
            fprintf(fp, "%s\n ", buf);
            strcpy(buf, buf1);
            len = len1 + 1;
        }
    }
    fprintf(fp, "%s;\n", buf);
}


// As above, to lstr.  The startlen is the number of characters in the
// current line in lstr when passed.
//
void
sCifGen::out_path(sLstr &lstr, const Point *points, int numpts, int startlen,
    int x, int y)
{
    int len = startlen;
    for (const Point *pair = points; numpts; pair++, numpts--) {
        char buf1[80];
        sprintf(buf1, " %d %d", pair->x - x, pair->y - y);
        int len1 = strlen(buf1);
        if (len + len1 < 79) {
            lstr.add(buf1);
            len += len1;
        }
        else {
            lstr.add_c('\n');
            lstr.add(buf1);
            len = len1 + 1;
        }
    }
    lstr.add(";\n");
}


// New 10/29/12, semicolons are ignored in extension strings if
// preceded by a backslash.  The backslash is stripped when read. 
// This function adds a backslash before semicolons, and will strip a
// trailing backslash that would cause trouble when the terminating
// semicolon is added.
//
void
sCifGen::fix_sc(sLstr &lstr, const char *str)
{
    if (str) {
        while (*str) {
            if (*str == ';')
                lstr.add_c('\\');
            lstr.add_c(*str);
            str++;
        }
    }
    if (!lstr.string())
        lstr.add_c(0, true);
}
// End of sCifGen functions


// Convert the hierarchies under the named symbols to CIF format
// according to the parameters.  The scale is used for the physical
// data.
//
// Return values:
//  OIok        normal return
//  OIerror     unspecified error (message in Errs)
//  OIabort     user interrupt
//
OItype
cFIO::ToCIF(const stringlist *namelist, const FIOcvtPrms *prms)
{
    if (!namelist) {
        Errs()->add_error("Error in ToCIF: null symbol list.");
        return (OIerror);
    }
    if (!prms) {
        Errs()->add_error("Error in ToCIF: null params pointer.");
        return (OIerror);
    }

    const char *cif_fname = prms->destination();
    if (!cif_fname || !*cif_fname) {
        Errs()->add_error("Error in ToCIF: null CIF file name.");
        return (OIerror);
    }

    if (!namelist->next && namelist->string &&
            !strcmp(namelist->string, FIO_CUR_SYMTAB)) {
        // Secret code to dump the entire current symbol table.

        cif_out *cif = new cif_out(0);
        GCobject<cif_out*> gc_cif(cif);
        if (!cif->set_destination(cif_fname))
            return (OIerror);
        cif->assign_alias(NewWritingAlias(prms->alias_mask(), false));
        cif->read_alias(cif_fname);
        double scale = prms->scale();
        if (!cif->allow_scale_extension())
            // always 100 units/micron in output
            scale *= 100.0/CDphysResolution;

        bool ok = cif->write_all(Physical, scale);
        if (ok && !fioStripForExport)
            ok = cif->write_all(Electrical, 1.0);
        return (ok ? OIok : cif->was_interrupted() ? OIaborted : OIerror);
    }

    for (const stringlist *sl = namelist; sl; sl = sl->next) {
        if (!CDcdb()->findSymbol(sl->string)) {
            Errs()->add_error("Error in ToCIF: %s not in symbol table.",
                sl->string ? sl->string : "(null)");
            return (OIerror);
        }
        if (prms->flatten())
            break;
    }

    cif_out *cif = new cif_out(0);
    GCobject<cif_out*> gc_cif(cif);
    if (!cif->set_destination(cif_fname))
        return (OIerror);
    cif->assign_alias(NewWritingAlias(prms->alias_mask(), false));
    cif->read_alias(cif_fname);

    double scale = prms->scale();
    const BBox *AOI = prms->use_window() ? prms->window() : 0;
    // Physical
    if (!cif->allow_scale_extension())
        // always 100 units/micron in output
        scale *= 100.0/CDphysResolution;
    bool ok;
    if (prms->flatten())
        // Just one name allowed, physical mode only.
        ok = cif->write_flat(namelist->string, scale, AOI, prms->clip());
    else {
        ok = cif->write(namelist, Physical, scale);
        if (ok && !fioStripForExport) {
            // Electrical
            ok = cif->write(namelist, Electrical, 1.0);
        }
    }
    return (ok ? OIok : cif->was_interrupted() ? OIaborted : OIerror);
}
// End of cCD functions


cif_out::cif_out(const char *srcfile)
{
    out_filetype = Fcif;
    out_fp = 0;
    out_cellname = 0;
    out_srcfile = lstring::copy(srcfile);
    out_layer_name_tab = 0;
    out_layer_map_tab = 0;
    out_layer_count = 0;
    out_layer_written = true;

    if (FIO()->IsStripForExport()) {
        unsigned int f = FIO()->CifStyle().flags_export();
        out_scale_extension = (f & CIF_SCALE_EXTENSION);
        out_cell_properties = (f & CIF_CELL_PROPERTIES);
        out_inst_name_comment = (f & CIF_INST_NAME_COMMENT);
        out_inst_name_extension = (f & CIF_INST_NAME_EXTENSION);
        out_inst_magn_extension = (f & CIF_INST_MAGN_EXTENSION);
        out_inst_array_extension = (f & CIF_INST_ARRAY_EXTENSION);
        out_inst_bound_extension = (f & CIF_INST_BOUND_EXTENSION);
        out_inst_properties = (f & CIF_INST_PROPERTIES);
        out_obj_properties = (f & CIF_OBJ_PROPERTIES);
        out_wire_extension = (f & CIF_WIRE_EXTENSION);
        out_wire_extension_new = (f & CIF_WIRE_EXTENSION_NEW);
        out_text_extension = (f & CIF_TEXT_EXTENSION);
    }
    else {
        unsigned int f = FIO()->CifStyle().flags();
        out_scale_extension = (f & CIF_SCALE_EXTENSION);
        out_cell_properties = (f & CIF_CELL_PROPERTIES);
        out_inst_name_comment = (f & CIF_INST_NAME_COMMENT);
        out_inst_name_extension = (f & CIF_INST_NAME_EXTENSION);
        out_inst_magn_extension = (f & CIF_INST_MAGN_EXTENSION);
        out_inst_array_extension = (f & CIF_INST_ARRAY_EXTENSION);
        out_inst_bound_extension = (f & CIF_INST_BOUND_EXTENSION);
        out_inst_properties = (f & CIF_INST_PROPERTIES);
        out_obj_properties = (f & CIF_OBJ_PROPERTIES);
        out_wire_extension = (f & CIF_WIRE_EXTENSION);
        out_wire_extension_new = (f & CIF_WIRE_EXTENSION_NEW);
        out_text_extension = (f & CIF_TEXT_EXTENSION);
    }
    out_add_obj_bb = FIO()->CifStyle().add_obj_bb();
}


cif_out::~cif_out()
{
    clear_property_queue();
    if (out_fp)
        fclose(out_fp);
    delete [] out_cellname;
    delete [] out_srcfile;
    delete out_layer_name_tab;
    delete out_layer_map_tab;
}


bool
cif_out::check_for_interrupt()
{
    out_rec_count++;
    if (!(out_rec_count % 16)) {
        uint64_t sx = large_ftell(out_fp);
        if (sx > out_fb_thresh) {
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
cif_out::write_header(const CDs*)
{
    if (out_mode == Physical)
        fprintf(out_fp, "(%s);\n", CD()->ifIdString());
    if (!write_library(0, 1.0, 1.0, 0, 0, 0))
        return (false);
    return (true);
}


// Write an object, properties are already queued.
//
bool
cif_out::write_object(const CDo *odesc, cvLchk *lchk)
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

    if (odesc->type() == CDBOX) {
        if (!write_box(&odesc->oBB()))
            return (false);
    }
    else if (odesc->type() == CDPOLYGON) {
        const Poly po(((const CDpo*)odesc)->po_poly());
        if (!write_poly(&po))
            return (false);
    }
    else if (odesc->type() == CDWIRE) {
        const Wire w(((const CDw*)odesc)->w_wire());
        if (!write_wire(&w))
            return (false);
    }
    else if (odesc->type() == CDLABEL) {
        Text text;
        // use long text for unbound labels
        CDp_lref *prf = (CDp_lref*)odesc->prpty(P_LABRF);
        char *ltxt = ((CDla*)odesc)->label()->string(HYcvAscii,
            !(prf && prf->devref()));
        const Label la(((const CDla*)odesc)->la_label());
        bool ret = text.set(&la, out_mode, Fcif);
        text.text = ltxt;
        if (ret)
            ret = write_text(&text);
        delete [] text.text;
        if (!ret)
            return (false);
    }
    if (out_add_obj_bb) {
        // Add a comment giving the bounding box of the
        // object.
        BBox BB = odesc->oBB();
        if (out_needs_mult) {
            BB.left = scale(BB.left);
            BB.bottom = scale(BB.bottom);
            BB.right = scale(BB.right);
            BB.top = scale(BB.top);
        }
        fprintf(out_fp, "(Bound %d %d %d %d);\n",
            BB.left, BB.bottom, BB.right, BB.top);
    }
    return (true);
}


bool
cif_out::set_destination(const char *destination)
{
    out_fp = FIO()->POpen(destination, "w");
    if (!out_fp) {
        Errs()->sys_error("open");
        Errs()->add_error("Unable to open %s.", destination);
        return (false);
    }
    out_filename = lstring::copy(destination);
    return (true);
}


bool
cif_out::queue_property(int val, const char *string)
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
cif_out::open_library(DisplayMode mode, double sc)
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
    out_cellname = 0;

    if (out_mode == Physical) {
        fprintf(out_fp, "(%s);\n", CD()->ifIdString());
        fprintf(out_fp, "(Translation from: %s);\n",
            out_srcfile ? out_srcfile : "unknown");
    }
    time_t tloc = time(0);
    tm now = *gmtime(&tloc);
    if (!write_library(0, 0, 0, &now, &now, 0))
        return (false);

    return (true);
}


bool
cif_out::write_library(int, double, double, tm*, tm*, const char*)
{
    if (out_mode == Physical) {
        if (!FIO()->IsStripForExport())
            fprintf(out_fp, "(PHYSICAL);\n");
        if (out_scale_extension) {
            if (CDphysResolution != 100)
                fprintf(out_fp, "(RESOLUTION %d);\n", CDphysResolution);
        }
    }
    else {
        fprintf(out_fp, "(ELECTRICAL);\n");
        if (out_scale_extension) {
            if (CDelecResolution != 100)
                fprintf(out_fp, "(RESOLUTION %d);\n", CDelecResolution);
        }
    }
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_struct(const char *name, tm*, tm*)
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

    // In the converter, out_visited is already created, and the
    // symnum should be in the table.  This is not true if
    // translating.
    //
    if (!out_visited)
        out_visited = new vtab_t(true);
    int symnum = out_visited->find(name);
    if (symnum < 0) {
        out_visited->add(name, out_symnum);
        symnum = out_symnum++;
    }
    name = alias(name);

    if (out_cell_properties) {
        // cell property extension
        // Note: electrical cells are never scaled.  Only the grid property
        // of physical cells is a candidate for scaling, but it is probably
        // not worth the effort.  In electrical cells, if the physical
        // cell is scaled, the physical terminal locations should also be
        // scaled, or the extract functions will be broken.
        //
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
            CDp *px = pd->dup();
            if (px) {
                px->scale(out_scale, out_phys_scale, out_mode);
                Gen.Property(out_fp, px->value(), px->string());
                delete px;
            }
        }
    }

    // write DS line and symbol name extension
    Gen.BeginSymbol(out_fp, symnum, 1, 1);
    if (out_mode == Electrical)
        fprintf(out_fp, "9 %s;\n", name);
    else {
        switch (FIO()->CifStyle().cname_type()) {
        case EXTcnameDef:
            fprintf(out_fp, "9 %s;\n", name);
            break;
        case EXTcnameNCA:
            fprintf(out_fp,"( %s );\n", name);
            break;
        case EXTcnameICARUS:
            fprintf(out_fp, "( 9 %s );\n", name);
            break;
        case EXTcnameSIF:
            fprintf(out_fp, "( Name: %s );\n", name);
            break;
        case EXTcnameNone:
        default:
            // don't add name extension
            break;
        }
    }
    delete [] out_cellname;
    out_cellname = lstring::copy(name);
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_end_struct(bool force)
{
    if (out_in_struct || force) {
        if (out_no_struct)
            return (true);
        Gen.EndSymbol(out_fp);
        out_in_struct = false;
    }
    return (ferror(out_fp) == 0);
}


bool
cif_out::queue_layer(const Layer *layer, bool*)
{
    if (!out_in_struct)
        return (true);

    if ((out_mode == Electrical) ||
            FIO()->CifStyle().layer_type() == EXTlayerDef) {
        if (!out_layer_name_tab)
            out_layer_name_tab = new SymTab(true, true);

        // All layer names are saved in the table.  The mapped-to name
        // is the data item, which is null if not mapped.

        const char *lname = 0;
        SymTabEnt *h = out_layer_name_tab->get_ent(layer->name);
        if (!h) {
#ifdef MAP_LNAMES
            if (strlen(layer->name) > 4) {
                char buf[8];
                for (;;) {
                    sprintf(buf, "L%03d", out_layer_count);
                    out_layer_count++;
                    if (out_layer_name_tab->get(buf) != ST_NIL)
                        continue;
                    break;
                }
                lname = lstring::copy(buf);
                if (!out_layer_map_tab)
                    out_layer_map_tab = new SymTab(false, false);
                out_layer_map_tab->add(lname, 0, false);
                out_layer_name_tab->add(lstring::copy(layer->name), lname,
                    false);
                FIO()->ifPrintCvLog(IFLOG_INFO, "Mapping layer %s to %s.",
                    layer->name, lname);
            }
            else {
#else
            {
#endif
                lname = lstring::copy(layer->name);
                out_layer_name_tab->add(lname, 0, false);
            }
        }
        else if (!h->stData) {
            if (!out_layer_map_tab ||
                    out_layer_map_tab->get(layer->name) == ST_NIL)
                lname = h->stTag;
            else {
                // oops, collision with a mapped name
                char buf[8];
                for (;;) {
                    sprintf(buf, "L%03d", out_layer_count);
                    out_layer_count++;
                    if (out_layer_name_tab->get(buf) != ST_NIL)
                        continue;
                    break;
                }
                lname = lstring::copy(buf);
                if (!out_layer_map_tab)
                    out_layer_map_tab = new SymTab(false, false);
                out_layer_map_tab->add(lname, 0, false);
                out_layer_name_tab->add(lstring::copy(layer->name), lname,
                    false);
                FIO()->ifPrintCvLog(IFLOG_INFO, "Mapping layer %s to %s.",
                    layer->name, lname);
            }
        }
        else {
            // lname is mapped name
            lname = (const char*)h->stData;
        }

        if (out_layer.name != lname || out_layer.index != layer->index) {
            out_layer.name = lname;
            out_layer.index = layer->index;
            out_layer_written = false;
        }
    }
    else {
        if (out_layer.index != layer->index) {
            out_layer.index = layer->index;
            out_layer_written = false;
        }
    }
    return (true);
}


bool
cif_out::write_box(const BBox *BB)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);
    if (!write_layer_rec())
        return (false);
    if (out_obj_properties) {
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
            Gen.Property(out_fp, pd->value(), pd->string());
    }

    if (out_needs_mult) {
        BBox tBB(scale(BB->left), scale(BB->bottom),
            scale(BB->right), scale(BB->top));
        Gen.Box(out_fp, tBB);
    }
    else
        Gen.Box(out_fp, *BB);
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_poly(const Poly *po)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);
    if (!write_layer_rec())
        return (false);
    if (out_obj_properties) {
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
            Gen.Property(out_fp, pd->value(), pd->string());
    }

    if (out_needs_mult) {
        Point *p = po->points;
        Point *points = new Point[po->numpts];
        for (int i = 0; i < po->numpts; i++)
            points[i].set(scale(p[i].x), scale(p[i].y));
        Gen.Polygon(out_fp, points, po->numpts);
        delete [] points;
    }
    else
        Gen.Polygon(out_fp, po->points, po->numpts);
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_wire(const Wire *wire)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);
    if (!write_layer_rec())
        return (false);
    if (out_obj_properties) {
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
            Gen.Property(out_fp, pd->value(), pd->string());
    }

    if (out_wire_extension || out_wire_extension_new) {
        if (out_needs_mult) {
            Point *p = wire->points;
            Point *points = new Point[wire->numpts];
            for (int i = 0; i < wire->numpts; i++)
                points[i].set(scale(p[i].x), scale(p[i].y));
            int width = scale(wire->wire_width());
            if (out_wire_extension_new)
                Gen.Wire(out_fp, width, wire->wire_style(), points,
                    wire->numpts);
            else {
                if (wire->wire_style() != CDWIRE_EXTEND)
                    fprintf(out_fp, "5 7033 PATHTYPE %d;\n",
                        wire->wire_style());
                Gen.Wire(out_fp, width, -1, points, wire->numpts);
            }
            delete [] points;
        }
        else {
            if (out_wire_extension_new)
                Gen.Wire(out_fp, wire->wire_width(), wire->wire_style(),
                    wire->points, wire->numpts);
            else {
                if (wire->wire_style() != CDWIRE_EXTEND)
                    fprintf(out_fp, "5 7033 PATHTYPE %d;\n",
                        wire->wire_style());
                Gen.Wire(out_fp, wire->wire_width(), -1, wire->points,
                    wire->numpts);
            }
        }
        return (ferror(out_fp) == 0);
    }

    // Writing traditional CIF

    Point *rpts = 0, *pts = wire->points;
    int num = wire->numpts;
    int width = scale(wire->wire_width());
    bool clipped1 = false;
    const char *msg1 = "Pathtype 0 vertex clipped in cell %s near %d %d.";
    const char *msg2 =
        "Pathtype 0 conversion to box or poly in cell %s near %d %d.";

    // The pathtype attribute is lost in conversion.
    // A wire from CIF is taken as pathtype 2 in cd, but
    // other systems may take them as pathtype 1.
    // A pathtype 0 (butt-end) wire is converted to
    // pathtype 2 or a polygon here before conversion.
    //
    if (wire->wire_style() == CDWIRE_FLUSH || out_needs_mult) {
        rpts = new Point[num];
        for (int i = 0; i < num; i++)
            rpts[i].set(scale(pts[i].x), scale(pts[i].y));
        pts = rpts;
    }
    if (width == 0 || wire->wire_style() != CDWIRE_FLUSH) {
        Gen.Wire(out_fp, width, -1, pts, num);
        delete [] rpts;
        return (ferror(out_fp) == 0);
    }

    // converting from 0 to 1, 2
    width /= 2;

    int d;
    if (pts[0].x == pts[1].x)
        d = abs(pts[0].y - pts[1].y);
    else if (pts[0].y == pts[1].y)
        d = abs(pts[0].x - pts[1].x);
    else
        d = (int)(sqrt((pts[0].x - pts[1].x)*(double)(pts[0].x - pts[1].x) +
            (pts[0].y - pts[1].y)*(double)(pts[0].y - pts[1].y)));
    if (d > 2*width)
        Wire::convert_end(&pts[0].x, &pts[0].y, pts[1].x, pts[1].y, width,
            true);
    else {
        if (num == 2) {
            // create a box or poly
            FIO()->ifPrintCvLog(IFLOG_WARN, msg2, out_cellname, pts[0].x,
                pts[0].y);
            if (!convert_2v(pts, width)) {
                Errs()->add_error(
                    "Internal error: wire conversion failed.");
                return (false);
            }
            delete [] rpts;
            return (true);
        }
        else if (d > width)
            Wire::convert_end(&pts[0].x, &pts[0].y, pts[1].x, pts[1].y,
                width, true);
        else {
            // Can't retract first vertex, clip it.
            pts++;
            num--;
            FIO()->ifPrintCvLog(IFLOG_WARN, msg1, out_cellname, pts[0].x,
                pts[0].y);
            clipped1 = true;
        }
    }

    if (pts[num-1].x == pts[num-2].x)
        d = abs(pts[num-1].y - pts[num-2].y);
    if (pts[num-1].y == pts[num-2].y)
        d = abs(pts[num-1].x - pts[num-2].x);
    else
        d = (int)(sqrt((pts[num-1].x - pts[num-2].x)*
                (double)(pts[num-1].x - pts[num-2].x) +
            (pts[num-1].y - pts[num-2].y)*
                (double)(pts[num-1].y - pts[num-2].y)));
    if (d > 2*width)
        Wire::convert_end(&pts[num-1].x, &pts[num-1].y, pts[num-2].x,
            pts[num-2].y, width, true);
    else {
        if (num == 2 && clipped1) {
            // clipped first vertex of three above
            FIO()->ifPrintCvLog(IFLOG_WARN, msg2, out_cellname, pts[num-1].x,
                pts[num-1].y);
            if (!convert_2v(pts, width)) {
                Errs()->add_error(
                    "Internal error: wire conversion failed.");
                return (false);
            }
            delete [] rpts;
            return (true);
        }
        else if (d > width)
            Wire::convert_end(&pts[num-1].x, &pts[num-1].y, pts[num-2].x,
                pts[num-2].y, width, true);
        else {
            // Can't retract last vertex, clip it.
            num--;
            FIO()->ifPrintCvLog(IFLOG_WARN, msg1, out_cellname, pts[num-1].x,
                pts[num-1].y);
        }
    }
    width *= 2;
    Gen.Wire(out_fp, width, -1, pts, num);
    delete [] rpts;
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_text(const Text *text)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);

    if (!text->text || !*text->text)
        return (true);
    if (!write_layer_rec())
        return (false);

    char *string;
    if (out_text_extension) {
        string = new char[strlen(text->text) + 5];
        strcpy(string, "<<");
        strcat(string, text->text);
        strcat(string, ">>");
    }
    else
        string = lstring::copy(text->text);
    if (out_mode == Electrical && out_needs_mult)
        string = hyList::hy_scale(string, out_scale);

    char fillc = out_text_extension ? ' ' : '_';
    for (char *s = string; *s; s++) {
        if (*s == ';')
            *s = fillc;
        else if (!out_text_extension && isspace(*s))
            // "real" cif has no spaces
            *s = fillc;
    }

    // write property list extension
    if (out_obj_properties) {
        for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
            Gen.Property(out_fp, pd->value(), pd->string());
    }
    if (out_mode == Electrical ||
            FIO()->CifStyle().label_type() == EXTlabelDef) {
        if (FIO()->IsStripForExport() || text->width <= 0)
            // Simple CD label
            fprintf(out_fp, "94 %s %d %d;\n", string,
                scale(text->x), scale(text->y));
        else
            // CD label
            Gen.Label(out_fp, string, scale(text->x), scale(text->y),
                text->xform, scale(text->width), scale(text->height));
    }
    else if (FIO()->CifStyle().label_type() == EXTlabelKIC)
        // Simple CD label
        fprintf(out_fp, "94 %s %d %d;\n", string, scale(text->x),
            scale(text->y));
    else if (FIO()->CifStyle().label_type() == EXTlabelNCA)
        // NCA label
        fprintf(out_fp, "92 %s %d %d %d;\n", string, scale(text->x),
            scale(text->y), out_layer.index);
    else if (FIO()->CifStyle().label_type() == EXTlabelMEXTRA)
        // mextra label
        fprintf(out_fp, "94 %s %d %d %s;\n", string, scale(text->x),
            scale(text->y), out_layer.name);
    // else no label at all
    delete [] string;
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_sref(const Instance *inst)
{
    if (!check_for_interrupt())
        return (false);
    if (!out_in_struct)
        return (true);

    const char *cellname = alias(inst->name);

    // In the converter, out_visited is already created, and the
    // symnums should be in the table.  This is not true if
    // translating.
    //
    if (!out_visited)
        out_visited = new vtab_t(true);
    int symnum = out_visited->find(cellname);
    if (symnum < 0) {
        out_visited->add(cellname, out_symnum);
        symnum = out_symnum++;
    }

    if (out_inst_array_extension) {
        if (out_inst_name_comment)
            fprintf(out_fp, "(SymbolCall %s);\n", cellname);
        if (out_inst_properties) {
            // add property list info
            for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
                CDp *px = pd->dup();
                if (px) {
                    px->scale(out_scale, out_phys_scale, out_mode);
                    Gen.Property(out_fp, px->value(), px->string());
                    delete px;
                }
            }
        }
        if (out_inst_bound_extension) {
            // add symbol bounding box extension, if possible
            if (inst->cdesc)
                fprintf(out_fp, "1 Bound %d %d %d %d;\n",
                    inst->cdesc->oBB().left, inst->cdesc->oBB().bottom,
                    inst->cdesc->oBB().right, inst->cdesc->oBB().top);
        }
        if (out_inst_magn_extension) {
            // add symbol scale extension
            if (inst->magn != 1.0)
                fprintf(out_fp, "1 Magnify %.16f;\n", inst->magn);
        }
        else {
            if (inst->magn != 1.0) {
                Errs()->add_error(
                    "Non-unit subcell magnification found.\n"
                    "  Magnification extension is disabled, abort.");
                return (false);
            }
        }

        // add symbol array extension
        if (inst->nx != 1 || inst->ny != 1)
            fprintf(out_fp, "1 Array %d %d %d %d;\n",
                inst->nx, scale(inst->dx), inst->ny, scale(inst->dy));

        if (out_inst_name_extension)
            // add symbol name extension
            fprintf(out_fp, "9 %s;\n", cellname);

        Gen.BeginCall(out_fp, symnum);
        Gen.MirrorY(out_fp, inst->reflection);
        Gen.Rotation(out_fp, inst->ax, inst->ay);
        Gen.Translation(out_fp, scale(inst->origin.x), scale(inst->origin.y));
        Gen.EndCall(out_fp);
    }
    else {
        int dx = scale(inst->dx);
        int dy = scale(inst->dy);
        if (out_inst_magn_extension && inst->magn != 1.0) {
            dx = mmRnd(inst->magn*dx);
            dy = mmRnd(inst->magn*dy);
        }
        for (int i = 0; i < inst->ny; i++) {
            for (int j = 0; j < inst->nx; j++) {

                if (out_inst_name_comment)
                    fprintf(out_fp, "(SymbolCall %s);\n", cellname);
                if (out_inst_properties) {
                    // add property list info
                    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
                        CDp *px = pd->dup();
                        if (px) {
                            px->scale(out_scale, out_phys_scale, out_mode);
                            Gen.Property(out_fp, px->value(), px->string());
                            delete px;
                        }
                    }
                }
                if (out_inst_magn_extension) {
                    // add symbol scale extension
                    if (inst->magn != 1.0)
                        fprintf(out_fp, "1 Magnify %.16f;\n", inst->magn);
                }
                else {
                    if (inst->magn != 1.0) {
                        Errs()->add_error(
                            "Non-unit subcell magnification found.\n"
                            "  Magnification extension is disabled, abort.");
                        return (false);
                    }
                }
                if (out_inst_name_extension)
                    // add symbol name extension
                    fprintf(out_fp, "9 %s;\n", cellname);

                Gen.BeginCall(out_fp, symnum);
                Gen.Translation(out_fp, j*dx, i*dy);
                Gen.MirrorY(out_fp, inst->reflection);
                Gen.Rotation(out_fp, inst->ax, inst->ay);
                Gen.Translation(out_fp, scale(inst->origin.x),
                    scale(inst->origin.y));
                Gen.EndCall(out_fp);
            }
        }
    }
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_endlib(const char *topcell_name)
{
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    if (topcell_name) {
        // Write a transform-less symbol call to the toplevel cell. 
        // MOSIS specifically requires this.
        int symnum = out_visited->find(topcell_name);
        if (symnum >= 0)
            fprintf(out_fp, "C %d;\n", symnum);
    }
    fprintf(out_fp, "End\n");
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_info(Attribute*, const char*)
{
    return (true);
}
// End of virtual overrides


// Function to begin write of a native cell file.
//
bool
cif_out::begin_native(const char *name)
{
    out_in_struct = true;
    out_mode = FIO()->ifCurrentDisplayMode();
    fprintf(out_fp, "(Symbol %s);\n", name);
    write_library(0, 1.0, 1.0, 0, 0, 0);
    fprintf(out_fp, "9 %s;\n", name);
    Gen.BeginSymbol(out_fp, 0, 1, 1);
    return (ferror(out_fp) == 0);
}


// Function to end write of a native cell file, replaces write_endlib.
//
bool
cif_out::end_native()
{
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    Gen.End(out_fp);
    return (ferror(out_fp) == 0);
}


bool
cif_out::write_layer_rec()
{
    if (!out_layer_written && out_in_struct) {
        out_layer_written = true;
        if ((out_mode == Electrical) ||
                FIO()->CifStyle().layer_type() == EXTlayerDef)
            Gen.Layer(out_fp, out_layer.name);
        else
            fprintf(out_fp, "L %d;\n", out_layer.index);
    }
    return (ferror(out_fp) == 0);
}


// Convert the length 2 list of points to box or polygon.
// int width;  half wire width
//
bool
cif_out::convert_2v(Point *pts, int width)
{
    BBox BB;
    if (pts[0].x == pts[1].x) {
        BB.left = pts[0].x - width;
        BB.bottom = mmMin(pts[0].y, pts[1].y);
        BB.right = pts[0].x + width;
        BB.top = mmMax(pts[0].y, pts[1].y);
        Gen.Box(out_fp, BB);
    }
    else if (pts[0].y == pts[1].y) {
        BB.left = mmMin(pts[0].x, pts[1].x);
        BB.bottom = pts[0].y - width;
        BB.right = mmMax(pts[0].x, pts[1].x);
        BB.top = pts[0].y + width;
        Gen.Box(out_fp, BB);
    }
    else {
        Wire wire(2*width, CDWIRE_FLUSH, 2, pts);
        Point *ppts;
        int pnum;
        if (!wire.toPoly(&ppts, &pnum))
            return (false);
        Gen.Polygon(out_fp, ppts, pnum);
        delete [] ppts;
    }
    return (true);
}

