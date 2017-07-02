
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
 $Id: fio_chd_split.cc,v 1.59 2015/10/11 19:35:45 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "fio.h"
#include "fio_gencif.h"
#include "fio_cvt_base.h"
#include "fio_chd.h"
#include "fio_chd_flat.h"
#include "fio_chd_cvtab.h"
#include "fio_chd_ecf.h"
#include "cd_digest.h"
#include "cd_chkintr.h"
#include "geo_ylist.h"
#include "timedbg.h"
#include <errno.h>


namespace {
    // Return a list of boxes obtained from the string, wich has format
    // l,b,r,t[,l,b,r,t]...
    //
    Blist *parse_rects(const char *str)
    {
        Blist *bl0 = 0, *be = 0;
        int vals[4];
        int cnt = 0;
        char *tok;
        while ((tok = lstring::gettok(&str, ",")) != 0) {
            char *ep;
            double d = strtod(tok, &ep);
            if (ep == tok) {
                // Parse error.
                delete [] tok;
                Blist::destroy(bl0);
                return (0);
            }
            delete [] tok;
            vals[cnt++] = INTERNAL_UNITS(d);
            if (cnt == 4) {
                cnt = 0;
                if (!bl0)
                    bl0 = be = new Blist;
                else {
                    be->next = new Blist;
                    be = be->next;
                }
                be->BB.left = vals[0];
                be->BB.bottom = vals[1];
                be->BB.right = vals[2];
                be->BB.top = vals[3];
                be->BB.fix();
            }
        }
        if (cnt != 0) {
            // Extra/missing numbers.
            Blist::destroy(bl0);
            return (0);
        }
        return (bl0);
    }
}


#define matching(s) lstring::cieq(tok, s)

// args: -i fname -o bname.ext [-c cname] -g grid | -r l,b,r,t[,l,b,r,t]...
//   [-b bloat] [-w l,b,r,t] [-f] [-m] [-cl] [-e[N]] [-p]
//
OItype
cFIO::SplitArchive(const char *string)
{
    if (!string) {
        Errs()->add_error("SplitArchive: null argument.");
        return (OIerror);
    }
    while (isspace(*string))
        string++;
    if (!*string) {
        Errs()->add_error("SplitArchive: empty argument.");
        return (OIerror);
    }

    char *fname = 0;
    char *bname = 0;
    char *cname = 0;
    int gridsize = 0;
    int bloatval = 0;
    BBox AOI;
    bool use_win = false;
    bool flatten = false;
    bool clip = false;
    ECFlevel ecf_level = ECFnone;
    bool flat_map = false;
    bool parallel = false;
    Blist *bl0 = 0;

    bool ret = true;
    char *tok;
    while ((tok = lstring::gettok(&string)) != 0) {
        const char *msg = "SplitArchive: missing %s argument.";

        if (matching("-i")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-f");
                ret = false;
                break;
            }
            fname = tok;
            tok = 0;
        }
        else if (matching("-o")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-o");
                ret = false;
                break;
            }
            bname = tok;
            tok = 0;
        }
        else if (matching("-c")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-c");
                ret = false;
                break;
            }
            cname = tok;
            tok = 0;
        }
        else if (matching("-g")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-g");
                ret = false;
                break;
            }
            double g;
            if (sscanf(tok, "%lf", &g) != 1) {
                delete [] tok;
                Errs()->add_error("SplitArchive: can't parse -g argument.");
                ret = false;
                break;
            }
            gridsize = INTERNAL_UNITS(g);
        }
        else if (matching("-r")) {
            // Regions can be comcatenated, or multiple -r specs can
            // be given.

            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-r");
                ret = false;
                break;
            }
            Blist *bl = parse_rects(tok);
            if (!bl) {
                delete [] tok;
                Errs()->add_error(
                    "SplitArchive: parse error in -r rectangle list.");
                ret = false;
                break;
            }
            if (!bl0)
                bl0 = bl;
            else {
                Blist *bx = bl0;
                while (bx->next)
                    bx = bx->next;
                bx->next = bl;
            }
        }
        else if (matching("-b")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-b");
                ret = false;
                break;
            }
            double b;
            if (sscanf(tok, "%lf", &b) != 1) {
                delete [] tok;
                Errs()->add_error("SplitArchive: can't parse -b argument.");
                ret = false;
                break;
            }
            bloatval = INTERNAL_UNITS(b);
        }
        else if (matching("-w")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-w");
                ret = false;
                break;
            }
            double l, b, r, t;
            if (sscanf(tok, "%lf,%lf,%lf,%lf", &l, &b, &r, &t) != 4) {
                delete [] tok;
                Errs()->add_error("SplitArchive: can't parse -w argument.");
                ret = false;
                break;
            }
            AOI.left = INTERNAL_UNITS(l);
            AOI.bottom = INTERNAL_UNITS(b);
            AOI.right = INTERNAL_UNITS(r);
            AOI.top = INTERNAL_UNITS(t);
            AOI.fix();
            use_win = true;
        }
        else if (matching("-f"))
            flatten = true;
        else if (matching("-m"))
            flat_map = true;
        else if (matching("-cl"))
            clip = true;

        else if (matching("-e0"))
            ;
        else if (matching("-e1") || matching("-e"))
            ecf_level = ECFall;
        else if (matching("-e2"))
            ecf_level = ECFpre;
        else if (matching("-e3"))
            ecf_level = ECFpost;

        else if (matching("-p"))
            parallel = true;
        else {
            Errs()->add_error("SplitArchive: unknown token %s.", tok);
            delete [] tok;
            ret = false;
            break;
        }
        delete [] tok;
    }

    if (ret && !fname) {
        Errs()->add_error("SplitArchive: no source name (-i) given.");
        ret = false;
    }
    if (ret && !bname) {
        Errs()->add_error("SplitArchive: no basename.ext (-o) given.");
        ret = false;
    }

    FileType oft = TypeExt(bname);
    if (oft != Fcif && oft != Fcgx && oft != Fgds && oft != Foas) {
        Errs()->add_error(
            "SplitArchive: unknown or invalid type extension given.");
        ret = false;
    }

    // If filtering layers, save plpc info for empty cell
    // pre-filtering.
    bool do_lf = UseLayerList() != ULLnoList && LayerList();

    cCHD *chd = 0;
    bool free_chd = false;
    if (ret) {
        char *ext = strrchr(bname, '.');
        *ext++ = 0;
        if (!strcmp(ext, "gz")) {
            ext = strrchr(bname, '.');
            *ext = 0;
        }

        chd = CDchd()->chdRecall(fname, false);
        if (!chd) {
            char *realname;
            FILE *fp = POpen(fname, "r", &realname);
            if (fp) {
                FileType ft = GetFileType(fp);
                fclose(fp);
                if (IsSupportedArchiveFormat(ft)) {
                    chd = NewCHD(realname, ft, Physical, 0,
                        do_lf ? cvINFOplpc : cvINFOtotals);
                    free_chd = true;
                }
                else {
                    sCHDin chd_in;
                    if (!chd_in.check(realname)) {
                        Errs()->add_error(
                            "SplitArchive: input file has unknown format.");
                        ret = false;
                    }
                    else {
                        chd = chd_in.read(realname,
                            sCHDin::get_default_cgd_type());
                        free_chd = true;
                    }
                }
            }
            delete [] realname;
        }
        if (!chd) {
            Errs()->add_error(
                "SplitArchive: could not obtain CHD for input.");
            ret = false;
        }
    }

    OItype oiret = ret ? OIok : OIerror;
    if (ret) {
        FIOcvtPrms prms;
        prms.set_allow_layer_mapping(true);
        prms.set_destination(bname, oft);
        if (use_win) {
            prms.set_use_window(true);
            prms.set_window(&AOI);
        }
        prms.set_flatten(flatten);
        prms.set_clip(clip);
        prms.set_ecf_level(ecf_level);

        oiret = chd->writeMulti(cname, &prms, bl0, gridsize, bloatval,
            CDMAXCALLDEPTH, flat_map, parallel);
    }

    if (free_chd)
        delete chd;
    delete [] fname;
    delete [] cname;
    delete [] bname;
    Blist::destroy(bl0);
    return (oiret);
}


namespace {
    void
    write_native_composite(const char *bname, stringlist *s0)
    {
        char *cname = new char[strlen(bname) + 32];
        strcpy(cname, bname);
        char *t = strrchr(cname, '.');
        if (t && lstring::cieq(t, ".gz"))
            *t = 0;
        else
            t = cname + strlen(cname);
        strcpy(t, "_root");

        char buf[256];
        FILE *fp = fopen(cname, "w");
        if (!fp)
            return;
        sprintf(buf, "Symbol %s", lstring::strip_path(cname));
        Gen.Comment(fp, buf);
        Gen.Comment(fp, "PHYSICAL");
        Gen.Comment(fp, "RESOLUTION 1000");
        Gen.UserExtension(fp, '9', lstring::strip_path(cname));
        Gen.BeginSymbol(fp, 0, 1, 1);

        for (stringlist *s = s0; s; s = s->next) {
            Gen.UserExtension(fp, '9', s->string);
            Gen.BeginCall(fp, 0);
            Gen.EndCall(fp);
        }
        Gen.EndSymbol(fp);
        Gen.End(fp);
        fclose(fp);
    }
}


// Write out the grid areas, output can be flat or hierarchical.
//
// cellname
//   Name  or top-level cell, iff null use default.
//
// blist
//   List of regions to process, grid is ignored if blist given.
//
// gridsize
//   The grid size, internal units.  Grid is square.
//
// bloatval
//   The areas will be bloated by this value, can be negative.
//
// max_flat_depth
//   The maximum depth to traverse when flattening.
//
// flat_map
//   When flattening, add _N to name of top-level cell, so names are
//   unique in output file collection.
//
// parallel
//   Use parallel writing algorithm
//
// iprms->destination()
// Base name for output files:  bname_nx_ny.ext, if null, use
// top-level cell name.  Can have a ".gz" suffix to indicate compression
// for GDSII/CGX output.
//
// iprms.scale()
//   Apply scale factor to data befroe processing.  The gridsize, bloatval,
//   and iprms->regionBB() are coordinates in the output.
//
// iprms->use_win(), iprms->regionBB()
//   Region to write, if not set use entire top-level cell BB.
//
// iprms->filetype()
//   Format of files to write: Fcif, Fcif, Fgds, Foas.
//
// iprms->flatten()
//   Output files will be flat.
//
// iprms->clip()
//   Geometry will be clipped to region.
//
// iprms->ecf_level()
//   Empty cell filtering level.
//
OItype
cCHD::writeMulti(const char *cellname, const FIOcvtPrms *iprms,
    const Blist *blist, unsigned int gridsize, int bloatval,
    unsigned int max_flat_depth, bool flat_map, bool parallel)
{
    TimeDbg tdbg("chd_write_multi");

    if (!iprms) {
        Errs()->add_error("writeMulti: null parameters pointer.");
        return (OIerror);
    }
    if (iprms->scale() < .001 || iprms->scale() > 1000.0) {
        Errs()->add_error("writeMulti: bad scale.");
        return (OIerror);
    }

    symref_t *ptop = findSymref(cellname, Physical, true);
    if (!ptop) {
        Errs()->add_error("writeMulti: unresolved top cell.");
        return (OIerror);
    }
    if (!blist) {
        if (gridsize <= 10) {
            Errs()->add_error("writeMulti: grid spacing too small.");
            return (OIerror);
        }
        if (abs(bloatval) > (int)gridsize/2) {
            Errs()->add_error("writeMulti: bad bloat value.");
            return (OIerror);
        }
    }
    if (!FIO()->IsSupportedArchiveFormat(iprms->filetype())) {
        Errs()->add_error("writeMulti: unsupported file type.");
        return (OIerror);
    }
    if (!setBoundaries(ptop)) {
        Errs()->add_error("writeMulti: setBoundaries failed.");
        return (OIerror);
    }

    BBox tcBB(*ptop->get_bb());
    if (iprms->scale() != 1.0)
        tcBB.scale(iprms->scale());
    BBox BB(iprms->use_window() ? *iprms->window() : *ptop->get_bb());
    if (iprms->scale() != 1.0 && !iprms->use_window())
        BB.scale(iprms->scale());
    if (iprms->use_window()) {
        if (!tcBB.intersect(&BB, false)) {
            Errs()->add_error("writeMulti: AOI does not overlap cell.");
            return (OIerror);
        }
    }
    int good_ones = 0;
    Blist *bl0 = 0, *be = 0;
    if (blist) {
        for (const Blist *bl = blist; bl; bl = bl->next) {
            BBox tBB(bl->BB);
            if (bloatval)
                tBB.bloat(bloatval);
            if (tBB.width() > 0 && tBB.height() > 0 &&
                    BB.intersect(&tBB, false)) {
                if (!bl0)
                    bl0 = be = new Blist(&tBB);
                else {
                    be->next = new Blist(&tBB);
                    be = be->next;
                }
                if (be->BB.left < BB.left)
                    be->BB.left = BB.left;
                if (be->BB.bottom < BB.bottom)
                    be->BB.bottom = BB.bottom;
                if (be->BB.right > BB.right)
                    be->BB.right = BB.right;
                if (be->BB.top > BB.top)
                    be->BB.top = BB.top;
                good_ones++;
            }
            else {
                // Bad region, but keep a place holder so that the
                // file names still match the order given by the caller.
                if (!bl0)
                    bl0 = be = new Blist(&CDnullBB);
                else {
                    be->next = new Blist(&CDnullBB);
                    be = be->next;
                }
            }
        }

        // The bloatval has been applied to the reagions, and the
        // regions have been clipped to the AOI.
        bloatval = 0;
        gridsize = 0;
        if (!good_ones) {
            Errs()->add_error("writeMulti: no overlapping regions given.");
            return (OIerror);
        }
    }

    // If layer filtering, do the empty-cell pre-filtering, which sets
    // the CVemty flags in the symrefs in the hierarchy.  These are
    // cleared from the ecf destructor.
    //
    CVecFilt ecf;
    if (iprms->ecf_level() == ECFall || iprms->ecf_level() == ECFpre ||
            iprms->flatten())
        ecf.setup(this, ptop);

    if (parallel) {
        FIO()->ifSetWorking(true);
        OItype oiret;
        if (iprms->flatten())
            oiret = write_multi_flat(ptop, iprms, bl0, gridsize, bloatval,
                max_flat_depth, flat_map);
        else
            oiret = write_multi_hier(ptop, iprms, bl0, gridsize, bloatval);
        FIO()->ifSetWorking(false);
        Blist::destroy(bl0);
        return (oiret);
    }

    const char *ext = cFIO::GetTypeExt(iprms->filetype());
    char *bname = lstring::copy(iprms->destination());
    if (!bname)
        bname = lstring::copy(ptop->get_name()->string());
    GCarray<char*> gc_bname(bname);

    bool do_gz = false;
    char *t = strrchr(bname, '.');
    if (t && lstring::cieq(t, ".gz")) {
        do_gz = true;
        *t = 0;
    }

    FIOcvtPrms prms;
    prms.set_allow_layer_mapping(true);
    prms.set_scale(iprms->scale());
    prms.set_use_window(true);
    prms.set_flatten(iprms->flatten());
    prms.set_clip(iprms->clip());
    prms.set_ecf_level(iprms->ecf_level());

    // Create input for this CHD.
    //
    cv_in *in = newInput(prms.allow_layer_mapping());
    if (!in) {
        Errs()->add_error("writeMulti: main input channel creation failed.");
        return (OIerror);
    }

    // Set input aliasing to CHD aliasing.
    //
    in->assign_alias(new FIOaliasTab(true, false, c_alias_info));

    stringlist *s0 = 0, *se = 0;

    FIO()->ifSetWorking(true);
    OItype oiret = OIok;
    if (bl0) {
        int nvals = Blist::length(bl0);
        int cnt = 0;
        for (Blist *bl = bl0; bl; bl = bl->next) {
            cnt++;
            if (bl->BB == CDnullBB)
                continue;
            prms.set_window(&bl->BB);
            FileType ft = iprms->filetype();
            char *tbuf = new char[strlen(bname) + 32];
            sprintf(tbuf, "%s%d%s", bname, cnt, ext);
            if (do_gz && (ft == Fgds || ft == Fcgx))
                strcat(tbuf, ".gz");
            prms.set_destination(tbuf, ft);
            if (iprms->flatten() && flat_map) {
                if (!s0)
                    s0 = se = new stringlist(lstring::copy(tbuf), 0);
                else {
                    se->next = new stringlist(lstring::copy(tbuf), 0);
                    se = se->next;
                }
            }
            delete [] tbuf;

            // Create output channel.
            //
            if (!in->setup_destination(prms.destination(), prms.filetype(),
                    false)) {
                Errs()->add_error("writeMulti: destination setup failed.");
                oiret = OIerror;
                break;
            }

            // Apply back-end aliasing.
            //
            cv_out *out = in->backend();
            unsigned int mask = prms.alias_mask();
            if (ft == Fgds)
                mask |= CVAL_GDS;
            FIOaliasTab *out_alias = FIO()->NewWritingAlias(mask,
                prms.flatten() && flat_map);
            out->assign_alias(out_alias);

            if (prms.flatten()) {
                if (flat_map) {
                    const char *cn = ptop->get_name()->string();
                    tbuf = new char[strlen(cn) + 32];
                    sprintf(tbuf, "%s_%d", cn, cnt);
                    out_alias->set_alias(cn, tbuf);
                    delete [] tbuf;
                }
                oiret = flatten(ptop, in, max_flat_depth, &prms);
            }
            else
                oiret = write(ptop, in, &prms, true);
            delete out;

            FIO()->ifInfoMessage(IFMSG_INFO, "Completed %d of %d.",
                cnt, nvals);

            if (oiret != OIok)
                break;
            if (checkInterrupt()) {
                oiret = OIaborted;
                break;
            }
        }
        Blist::destroy(bl0);
    }
    else {
        int nxc = BB.width()/gridsize + (BB.width()%gridsize != 0);
        int nyc = BB.height()/gridsize + (BB.height()%gridsize != 0);
        int nvals = nxc*nyc;
        int cnt = 0;
        for (int ic = 0; ic < nyc; ic++) {
            for (int jc = 0; jc < nxc; jc++) {
                int cx = BB.left + jc*gridsize;
                int cy = BB.bottom + ic*gridsize;
                BBox gBB(cx, cy, cx + gridsize, cy + gridsize);
                if (gBB.right > BB.right)
                    gBB.right = BB.right;
                if (gBB.top > BB.top)
                    gBB.top = BB.top;
                gBB.bloat(bloatval);

                prms.set_window(&gBB);
                FileType ft = iprms->filetype();
                char *tbuf = new char[strlen(bname) + 32];
                sprintf(tbuf, "%s_%d_%d%s", bname, jc, ic, ext);
                if (do_gz && (ft == Fgds || ft == Fcgx))
                    strcat(tbuf, ".gz");
                prms.set_destination(tbuf, ft);
                if (iprms->flatten() && flat_map) {
                    if (!s0)
                        s0 = se = new stringlist(lstring::copy(tbuf), 0);
                    else {
                        se->next = new stringlist(lstring::copy(tbuf), 0);
                        se = se->next;
                    }
                }
                delete [] tbuf;

                // Create output channel.
                //
                if (!in->setup_destination(prms.destination(), prms.filetype(),
                        false)) {
                    Errs()->add_error("writeMulti: destination setup failed.");
                    oiret = OIerror;
                    break;
                }

                // Apply back-end aliasing.
                //
                cv_out *out = in->backend();
                unsigned int mask = prms.alias_mask();
                if (ft == Fgds)
                    mask |= CVAL_GDS;
                FIOaliasTab *out_alias = FIO()->NewWritingAlias(mask,
                    prms.flatten() && flat_map);
                out->assign_alias(out_alias);

                if (prms.flatten()) {
                    if (flat_map) {
                        const char *cn = ptop->get_name()->string();
                        tbuf = new char[strlen(cn) + 32];
                        sprintf(tbuf, "%s_%d", cn, cnt);
                        out_alias->set_alias(cn, tbuf);
                        delete [] tbuf;
                    }
                    oiret = flatten(ptop, in, max_flat_depth, &prms);
                }
                else
                    oiret = write(ptop, in, &prms, true);
                delete out;

                FIO()->ifInfoMessage(IFMSG_INFO, "Completed %d of %d.",
                    ic*nxc + jc + 1, nvals);

                if (oiret != OIok)
                    break;
                if (checkInterrupt()) {
                    oiret = OIaborted;
                    break;
                }
                cnt++;
            }
            if (oiret != OIok)
                break;
        }
    }

    //
    // Write a native cell that calls all of the pieces.
    //
    if (oiret == OIok && iprms->flatten() && flat_map)
        write_native_composite(bname, s0);
    s0->free();

    FIO()->ifSetWorking(false);
    delete in;
    return (oiret);
}


//-----------------------------------------------------------------------------
// Parallel splitting function for hierarchical output.

namespace fio_chd_split {
    struct mpx_out : public cv_out
    {
        mpx_out(int n, cCVtab *ctab, cv_out **ch, const FIOcvtPrms *prms)
            {
                mpx_scale = prms->scale();
                mpx_channels = ch;
                mpx_cellname = 0;
                mpx_ctab = ctab;
                mpx_chd = 0;
                mpx_symref = 0;
                mpx_bbs = 0;
                mpx_nchannels = n;
                mpx_clip = prms->clip();
            }

        ~mpx_out()
            {
                delete [] mpx_cellname;
                delete [] mpx_bbs;
            }

        void set_cur_chd(const cCHD *chd)
            {
                mpx_chd = chd;
            }

        bool write_header(const CDs*) { return (false); }
        bool write_object(const CDo*, cvLchk*) { return (false); }

        bool set_destination(const char*) { return (true); }
        bool set_destination(FILE*, void**, void**) { return (true); };

        bool open_library(DisplayMode, double);
        bool queue_property(int, const char*);
        bool write_library(int, double, double, tm*, tm*, const char*);
        bool write_struct(const char*, tm*, tm*);
        bool write_end_struct(bool = false);
        bool queue_layer(const Layer*, bool*);
        bool write_box(const BBox*);
        bool write_poly(const Poly*);
        bool write_wire(const Wire*);
        bool write_text(const Text*);
        bool write_sref(const Instance*);
        bool write_endlib(const char*);

        bool write_info(Attribute*, const char*) { return (true); }

    private:
        bool set_new_cell(const char*);
        const BBox *get_cached_bb(int);

        double          mpx_scale;          // translation scaling
        cv_out          **mpx_channels;     // channel backends
        char            *mpx_cellname;      // name of cell being output
        cCVtab          *mpx_ctab;          // bounding box table
        const cCHD      *mpx_chd;           // CHD for current cell
        const symref_t  *mpx_symref;        // symref for current cell
        const BBox      **mpx_bbs;          // cached BBs for mpx_symref
        int             mpx_nchannels;      // number of output channels
        bool            mpx_clip;           // clip objects to boundary
    };
}

using namespace fio_chd_split;


bool
mpx_out::open_library(DisplayMode m, double sc)
{
    if (m != Physical)
        return (false);
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->open_library(m, sc);
        if (!ret)
            break;
    }
    return (ret);
}


bool
mpx_out::queue_property(int, const char*)
{
    return (true);
}


bool
mpx_out::write_library(int version, double munit, double uunit, tm *mdate,
    tm *adate, const char *libname)
{
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->write_library(version, munit, uunit, mdate,
            adate, libname);
        if (!ret)
            break;
    }
    return (ret);
}


bool
mpx_out::write_struct(const char *name, tm *cdate, tm *mdate)
{
    if (mpx_cellname)
        return (false);
    if (!mpx_bbs)
        mpx_bbs = new const BBox*[mpx_nchannels];
    bool ret = true;
    if (set_new_cell(name)) {
        for (int i = 0; i < mpx_nchannels; i++) {
            if (!(mpx_bbs[i] = get_cached_bb(i)))
                continue;
            ret = mpx_channels[i]->write_struct(name, cdate, mdate);
            if (!ret)
                break;
        }
    }
    mpx_cellname = lstring::copy(name);
    return (ret);
}


bool
mpx_out::write_end_struct(bool force)
{
    if (!mpx_cellname)
        return (true);
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        if (!mpx_bbs[i])
            continue;
        ret = mpx_channels[i]->write_end_struct(force);
        if (!ret)
            break;
    }
    delete [] mpx_cellname;
    mpx_cellname = 0;
    set_new_cell(0);
    return (ret);
}


bool
mpx_out::queue_layer(const Layer *layer, bool *check_mapping)
{
    if (!mpx_cellname)
        return (false);
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        if (!mpx_bbs[i])
            continue;
        ret = mpx_channels[i]->queue_layer(layer, check_mapping);
        if (!ret)
            break;
    }
    return (ret);
}


bool
mpx_out::write_box(const BBox *BB)
{
    if (!mpx_cellname)
        return (false);
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *tBB = mpx_bbs[i];
        if (!tBB)
            continue;
        BBox BBaoi(*tBB);
        BBaoi.scale(mpx_scale);
        if (BB->intersect(&BBaoi, false)) {
            if (!mpx_clip || BBaoi >= *BB)
                ret = mpx_channels[i]->write_box(BB);
            else {
                Blist *bl0 = BB->clip_to(&BBaoi);
                if (bl0) {
                    ret = mpx_channels[i]->write_box(&bl0->BB);
                    Blist::destroy(bl0);
                }
            }
            if (!ret)
                break;
        }
    }
    return (ret);
}


bool
mpx_out::write_poly(const Poly *poly)
{
    if (!mpx_cellname)
        return (false);
    bool ret = true;
    BBox BBp;
    poly->computeBB(&BBp);
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *tBB = mpx_bbs[i];
        if (!tBB)
            continue;
        BBox BBaoi(*tBB);
        BBaoi.scale(mpx_scale);
        if (BBp.intersect(&BBaoi, false) &&
                poly->intersect(&BBaoi, false)) {
            if (!mpx_clip || BBaoi >= BBp)
                ret = mpx_channels[i]->write_poly(poly);
            else {
                Zlist *zl = poly->toZlist();
                Ylist *yl = new Ylist(new Zlist(&BBaoi));
                ret = (Zlist::zl_and(&zl, yl) == XIok);
                Ylist::destroy(yl);

                if (zl) {
                    PolyList *pl = Zlist::to_poly_list(zl);
                    for (PolyList *p = pl; p; p = p->next) {
                        if (p->po.is_rect()) {
                            BBox BBb(p->po.points);
                            ret = mpx_channels[i]->write_box(&BBb);
                        }
                        else
                            ret = mpx_channels[i]->write_poly(&p->po);
                        if (!ret)
                            break;
                    }
                    PolyList::destroy(pl);
                }
            }
            if (!ret)
                break;
        }
    }
    return (ret);
}


bool
mpx_out::write_wire(const Wire *wire)
{
    if (!mpx_cellname)
        return (false);
    Poly po;
    if (!wire->toPoly(&po.points, &po.numpts))
        return (false);

    bool ret = true;
    BBox BBp;
    po.computeBB(&BBp);
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *tBB = mpx_bbs[i];
        if (!tBB)
            continue;
        BBox BBaoi(*tBB);
        BBaoi.scale(mpx_scale);
        if (BBp.intersect(&BBaoi, false) && po.intersect(&BBaoi, false)) {
            if (!mpx_clip || BBaoi >= BBp)
                ret = mpx_channels[i]->write_wire(wire);
            else {
                Zlist *zl = po.toZlist();
                Ylist *yl = new Ylist(new Zlist(&BBaoi));
                ret = (Zlist::zl_and(&zl, yl) == XIok);
                Ylist::destroy(yl);

                if (zl) {
                    PolyList *pl = Zlist::to_poly_list(zl);
                    for (PolyList *p = pl; p; p = p->next) {
                        if (p->po.is_rect()) {
                            BBox BBb(p->po.points);
                            ret = mpx_channels[i]->write_box(&BBb);
                        }
                        else
                            ret = mpx_channels[i]->write_poly(&p->po);
                        if (!ret)
                            break;
                    }
                    PolyList::destroy(pl);
                }
            }
            if (!ret)
                break;
        }
    }

    delete [] po.points;
    return (ret);
}


bool
mpx_out::write_text(const Text *text)
{
    if (!mpx_cellname)
        return (false);
    if (out_mode == Physical && FIO()->IsNoReadLabels())
        return (true);

    BBox lBB(0, 0, text->width, text->height);
    cTfmStack stk;
    stk.TSetTransformFromXform(text->xform, text->width, text->height);
    stk.TTranslate(text->x, text->y);
    Poly po(5, 0);
    stk.TBB(&lBB, &po.points);
    stk.TPop();

    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *tBB = mpx_bbs[i];
        if (!tBB)
            continue;
        BBox BBaoi(*tBB);
        BBaoi.scale(mpx_scale);

        if (bound_isect(&BBaoi, &lBB, &po) &&
                (!mpx_clip || BBaoi >= lBB)) {
            ret = mpx_channels[i]->write_text(text);
            if (!ret)
                break;
        }
    }
    delete [] po.points;
    return (ret);
}


bool
mpx_out::write_sref(const Instance *inst)
{
    if (!mpx_cellname)
        return (false);

    symref_t *pinst = 0;
    for (int i = 0; ; i++) {
        cCHD *chd = mpx_ctab->get_chd(i);
        if (!chd)
            break;
        symref_t *p = chd->findSymref(inst->name, Physical, false);
        if (!p || !p->get_defseen())
            continue;
        pinst = p;
        break;
    }
    if (!pinst)
        return (false);

    cTfmStack stk;
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *tBB = mpx_bbs[i];
        if (!tBB)
            continue;
        BBox BBaoi(*tBB);
        BBaoi.scale(mpx_scale);

        cvtab_item_t *item = mpx_ctab->get(pinst, i);
        if (!item)
            continue;
        tBB = item->get_bb();
        BBox sBB(*tBB);
        sBB.scale(mpx_scale);

        if (inst->check_overlap(&stk, &sBB, &BBaoi)) {
            ret = mpx_channels[i]->write_sref(inst);
            if (!ret)
                break;
        }
    }
    return (ret);
}


bool
mpx_out::write_endlib(const char *topcell_name)
{
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->write_endlib(topcell_name);
        if (!ret)
            break;
    }
    return (ret);
}


// Return true and set cache parameters for new cell name.
//
bool
mpx_out::set_new_cell(const char *name)
{
    mpx_symref = 0;
    if (!name)
        return (true);
    for (int i = 0; ; i++) {
        cCHD *chd = mpx_ctab->get_chd(i);
        if (!chd)
            break;
        symref_t *p = chd->findSymref(name, Physical, false);
        if (!p || !p->get_defseen())
            continue;
        mpx_symref = p;
        break;
    }
    return (mpx_symref != 0);
}


// Return the indexed BBox for the cached symref, if it exists.
//
const BBox *
mpx_out::get_cached_bb(int ix)
{
    cvtab_item_t *item = mpx_ctab->get(mpx_symref, ix);
    if (!item)
        return (0);
    return (item->get_bb());
}
// End of mpx_out functions.


// In parallel, write out the grid areas, output is hierarchical.
//
// ptop
//   Top-level symref.
//
// blist
//   List of regions to process, grid is ignored if blist given.
//
// gridsize
//   The grid size, internal units.  Grid is square.
//
// bloatval
//   The areas will be bloated by this value, can be negative.
//
// prms->destination()
// Base name for output files:  bname_nx_ny.ext, if null, use
// top-level cell name.  Can have a ".gz" suffix to indicate compression
// for GDSII/CGX output.
//
// prms.scale()
//   Apply scale factor to data befroe processing.  The gridsize, bloatval,
//   and prms->regionBB() are coordinates in the output.
//
// prms->use_win(), prms->regionBB()
//   Region to write, if not set use entire top-level cell BB.
//
// prms->filetype()
//   Format of files to write: Fcif, Fcif, Fgds, Foas.
//
// prms->flatten()  IGNORED
//
// prms->clip()
//   Geometry will be clipped to region.
//
// prms->ecf_level()
//   Empty cell filtering level.
//
OItype
cCHD::write_multi_hier(symref_t *ptop, const FIOcvtPrms *prms,
    const Blist *blist, unsigned int gridsize, int bloatval)
{
    // Local struct for cleanup and timing info.
    struct wmh_cleanup : public TimeDbg
    {
        wmh_cleanup() : TimeDbg("chd_write_multi_hier")
            {
                ctab = 0;
                itab = 0;
                in = 0;
                channels = 0;
                out = 0;
                nvals = 0;
            }

        ~wmh_cleanup()
            {
                delete ctab;
                delete itab;
                delete in;
                if (channels) {
                    for (int i = 0; i < nvals; i++)
                        delete channels[i];
                    delete [] channels;
                }
                delete out;
            }
            
        cCVtab      *ctab;
        chd_intab   *itab;
        cv_in       *in;
        cv_out      **channels;
        mpx_out     *out;
        int         nvals;
    };
    wmh_cleanup wmc;

    if (!ptop) {
        Errs()->add_error("write_multi_hier: unresolved cellname");
        return (OIerror);
    }

    //
    // Set up table of intersect areas.
    //
    int nxc = 0;
    int nyc = 0;

    if (blist) {
        wmc.nvals = Blist::length(blist);
        wmc.ctab = new cCVtab(false, wmc.nvals);

        int cnt = 0;
        int chcnt = 0;
        for (const Blist *bl = blist; bl; bl = bl->next) {
            cnt++;
            if (bl->BB == CDnullBB)
                continue;

            BBox cBB(bl->BB);
            cBB.bloat(bloatval);
            cBB.scale(1.0/prms->scale());

            if (!wmc.ctab->build_BB_table(this, ptop, chcnt, &cBB)) {
                Errs()->add_error(
                    "write_multi_hier: cell table build failed.");
                return (OIerror);
            }
            chcnt++;
        }
    }
    else {
        BBox aoiBB(prms->use_window() ? *prms->window() : *ptop->get_bb());
        if (prms->scale() != 1.0 && !prms->use_window())
            aoiBB.scale(prms->scale());
        nxc = aoiBB.width()/gridsize + (aoiBB.width()%gridsize != 0);
        nyc = aoiBB.height()/gridsize + (aoiBB.height()%gridsize != 0);
        wmc.nvals = nxc*nyc;
        wmc.ctab = new cCVtab(false, wmc.nvals);

        int cnt = 0;
        for (int ic = 0; ic < nyc; ic++) {
            for (int jc = 0; jc < nxc; jc++) {
                int cur_cx = aoiBB.left + jc*gridsize;
                int cur_cy = aoiBB.bottom + ic*gridsize;
                BBox cBB(cur_cx, cur_cy, cur_cx + gridsize, cur_cy + gridsize);
                if (cBB.right > aoiBB.right)
                    cBB.right = aoiBB.right;
                if (cBB.top > aoiBB.top)
                    cBB.top = aoiBB.top;

                cBB.bloat(bloatval);
                cBB.scale(1.0/prms->scale());

                if (!wmc.ctab->build_BB_table(this, ptop, cnt, &cBB)) {
                    Errs()->add_error(
                        "write_multi_hier: cell table build failed.");
                    return (OIerror);
                }
                cnt++;
            }
        }
    }

    //
    // Set up the output channels.
    //
    const char *bname = prms->destination();
    if (!bname)
        bname = ptop->get_name()->string();
    char *fname = new char[strlen(bname) + 32];
    strcpy(fname, bname);

    bool do_gz = false;
    char *t = strrchr(fname, '.');
    if (t && lstring::cieq(t, ".gz")) {
        if (prms->filetype() == Fgds || prms->filetype() == Fcgx)
            do_gz = true;
        *t = 0;
    }
    char *fend = fname + strlen(fname);
    const char *ext = FIO()->GetTypeExt(prms->filetype());

    wmc.channels = new cv_out*[wmc.nvals];
    memset(wmc.channels, 0, wmc.nvals*sizeof(cv_out*));

    if (blist) {
        int cnt = 0;
        int chcnt = 0;
        for (const Blist *bl = blist; bl; bl = bl->next) {
            cnt++;
            if (bl->BB == CDnullBB)
                continue;
            sprintf(fend, "%d%s", cnt, ext);
            if (do_gz)
                strcat(fend, ".gz");

            wmc.channels[chcnt] =
                FIO()->NewOutput(c_filename, fname, prms->filetype());
            if (!wmc.channels[chcnt])
                return (OIerror);

            // Apply back-end aliasing.
            //
            unsigned int mask = prms->alias_mask();
            if (prms->filetype() == Fgds)
                mask |= CVAL_GDS;
            wmc.channels[chcnt]->assign_alias(FIO()->NewWritingAlias(mask,
                false));
            wmc.channels[chcnt]->read_alias(fname);
            chcnt++;
        }
    }
    else {
        int cnt = 0;
        for (int ic = 0; ic < nyc; ic++) {
            for (int jc = 0; jc < nxc; jc++) {

                sprintf(fend, "_%d_%d%s", jc, ic, ext);
                if (do_gz)
                    strcat(fend, ".gz");

                wmc.channels[cnt] =
                    FIO()->NewOutput(c_filename, fname, prms->filetype());
                if (!wmc.channels[cnt])
                    return (OIerror);

                // Apply back-end aliasing.
                //
                unsigned int mask = prms->alias_mask();
                if (prms->filetype() == Fgds)
                    mask |= CVAL_GDS;
                wmc.channels[cnt]->assign_alias(FIO()->NewWritingAlias(mask,
                    false));
                wmc.channels[cnt]->read_alias(fname);
                cnt++;
            }
        }
    }
    delete [] fname;

    //
    // Set up the input reader.
    //
    wmc.in = newInput(prms->allow_layer_mapping());
    if (!wmc.in) {
        Errs()->add_error("write_multi_hier: NewInput failed.");
        return (OIerror);
    }
    if (prms->use_window())
        wmc.in->set_clip(prms->clip());

    //
    // Set input aliasing to CHD aliasing.
    //
    wmc.in->assign_alias(new FIOaliasTab(true, false, c_alias_info));

    //
    // Set up additional CHD input channels, for library references.
    //
    wmc.itab = new chd_intab;
    for (int i = 0; ; i++) {
        cCHD *tchd = wmc.ctab->get_chd(i);
        if (!tchd)
            break;
        if (tchd == this) {
            wmc.itab->insert(tchd, wmc.in);
            wmc.itab->set_no_free(wmc.in);
            continue;
        }
        cv_in *new_in = tchd->newInput(prms->allow_layer_mapping());
        if (!new_in) {
            Errs()->add_error(
                "write_multi_hier: reference channel setup failed.");
            return (OIerror);
        }
        if (prms->use_window())
            new_in->set_clip(prms->clip());
        wmc.itab->insert(tchd, new_in);
    }

    //
    // Set up the back end.
    //
    wmc.out = new mpx_out(wmc.nvals, wmc.ctab, wmc.channels, prms);
    wmc.in->setup_backend(wmc.out);

    if (prms->ecf_level() == ECFall || prms->ecf_level() == ECFpost) {
        for (int i = 0; i < wmc.nvals; i++) {
            if (wmc.ctab->prune_empties(ptop, wmc.itab, i) != OIok) {
                Errs()->add_error(
                    "write_multi_hier: error scanning for empties.");
                return (OIerror);
            }
        }
    }

    //
    // Do the work.
    //

    cv_in *cin = wmc.in;
    cCHD *cchd = this;

    bool ok = true;
    if (!cin->chd_setup(cchd, 0, 0, Physical, prms->scale())) {
        Errs()->add_error("write_multi_hier: main channel setup failed.");
        ok = false;
    }
    if (ok && wmc.out) {
        if (!wmc.out->open_library(Physical, 1.0)) {
            Errs()->add_error(
                "write_multi_hier: main channel header write failed.");
            ok = false;
        }
    }
    wmc.out->set_cur_chd(cchd);

    if (ok) {
        for (int i = 0; ; i++) {
            cCHD *tchd = wmc.ctab->get_chd(i);
            if (!tchd)
                break;

            if (tchd != cchd) {
                cin->chd_finalize();
                FIOaliasTab *at = cin->extract_alias();
                cin = wmc.itab->find(tchd);
                cin->assign_alias(at);
                cchd = tchd;

                if (!cin->chd_setup(cchd, 0, 0, Physical, prms->scale())) {
                    Errs()->add_error(
                        "write_multi_hier: reference channel setup failed.");
                    ok = false;
                    break;
                }
                wmc.out->set_cur_chd(cchd);
            }

            syrlist_t *sy0 = wmc.ctab->symref_list(cchd);
            for (syrlist_t *s = sy0; s; s = s->next) {
                if (s->symref->should_skip())
                    continue;
                ok = cin->chd_read_cell(s->symref, true);
                if (!ok)
                    break;
            }
            syrlist_t::destroy(sy0);
            if (!ok)
                break;
        }
    }
    cin->chd_finalize();
    wmc.in->assign_alias(cin->extract_alias());

    if (ok && wmc.out) {
        if (!wmc.in->no_end_lib() &&
                !wmc.out->write_endlib(ptop->get_name()->string())) {
            Errs()->add_error(
                "write_multi_hier: write end lib failed.");
            ok = false;
        }
    }
    OItype oiret =
        ok ? OIok : cin->was_interrupted() ? OIaborted : OIerror;

    return (oiret);
}


//-----------------------------------------------------------------------------
// Parallel splitting function for flat output.

namespace fio_chd_split {
    struct mpx_flat_out : public cv_out
    {
        mpx_flat_out(int n, BBox *bnds, cv_out **ch, bool clip)
            {
                mpx_nchannels = n;
                mpx_channels = ch;
                mpx_bounds = bnds;
                mpx_clip = clip;
            }

        ~mpx_flat_out()
            {
            }

        bool write_header(const CDs*) { return (false); }
        bool write_object(const CDo*, cvLchk*) { return (false); }

        bool set_destination(const char*) { return (true); }
        bool set_destination(FILE*, void**, void**) { return (true); };

        bool open_library(DisplayMode, double);
        bool queue_property(int, const char*);
        bool write_library(int, double, double, tm*, tm*, const char*);
        bool write_struct(const char*, tm*, tm*);
        bool write_end_struct(bool = false);
        bool queue_layer(const Layer*, bool*);
        bool write_box(const BBox*);
        bool write_poly(const Poly*);
        bool write_wire(const Wire*);
        bool write_text(const Text*);
        bool write_sref(const Instance*);
        bool write_endlib(const char*);

        bool write_info(Attribute*, const char*) { return (true); }

    private:
        const BBox *get_bb(int ix) { return (mpx_bounds + ix); }

        int mpx_nchannels;          // number of output channels
        cv_out **mpx_channels;      // channel backends
        BBox *mpx_bounds;           // channel areas
        bool mpx_clip;              // clip to output
    };
}


bool
mpx_flat_out::open_library(DisplayMode m, double sc)
{
    if (m != Physical)
        return (false);
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->open_library(m, sc);
        if (!ret)
            break;
    }
    return (ret);
}


bool
mpx_flat_out::queue_property(int, const char*)
{
    return (true);
}


bool
mpx_flat_out::write_library(int version, double munit, double uunit,
    tm *mdate, tm *adate, const char *libname)
{
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->write_library(version, munit, uunit, mdate,
            adate, libname);
        if (!ret)
            break;
    }
    return (ret);
}


bool
mpx_flat_out::write_struct(const char *name, tm *cdate, tm *mdate)
{
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->write_struct(name, cdate, mdate);
        if (!ret)
            break;
    }
    return (ret);
}


bool
mpx_flat_out::write_end_struct(bool force)
{
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->write_end_struct(force);
        if (!ret)
            break;
    }
    return (ret);
}


bool
mpx_flat_out::queue_layer(const Layer *layer, bool *check_mapping)
{
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->queue_layer(layer, check_mapping);
        if (!ret)
            break;
    }
    return (ret);
}


bool
mpx_flat_out::write_box(const BBox *BB)
{
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *BBaoi = get_bb(i);
        if (BB->intersect(BBaoi, false)) {
            if (!mpx_clip || *BBaoi >= *BB)
                ret = mpx_channels[i]->write_box(BB);
            else {
                Blist *bl0 = BB->clip_to(BBaoi);
                if (bl0) {
                    ret = mpx_channels[i]->write_box(&bl0->BB);
                    Blist::destroy(bl0);
                }
            }
            if (!ret)
                break;
        }
    }
    return (ret);
}


bool
mpx_flat_out::write_poly(const Poly *poly)
{
    bool ret = true;
    BBox BBp;
    poly->computeBB(&BBp);
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *BBaoi = get_bb(i);
        if (BBp.intersect(BBaoi, false) && poly->intersect(BBaoi, false)) {
            if (!mpx_clip || *BBaoi >= BBp)
                ret = mpx_channels[i]->write_poly(poly);
            else {
                Zlist *zl = poly->toZlist();
                Ylist *yl = new Ylist(new Zlist(BBaoi));
                ret = (Zlist::zl_and(&zl, yl) == XIok);
                Ylist::destroy(yl);

                if (zl) {
                    PolyList *pl = Zlist::to_poly_list(zl);
                    for (PolyList *p = pl; p; p = p->next) {
                        if (p->po.is_rect()) {
                            BBox BBb(p->po.points);
                            ret = mpx_channels[i]->write_box(&BBb);
                        }
                        else
                            ret = mpx_channels[i]->write_poly(&p->po);
                        if (!ret)
                            break;
                    }
                    PolyList::destroy(pl);
                }
            }
            if (!ret)
                break;
        }
    }
    return (ret);
}


bool
mpx_flat_out::write_wire(const Wire *wire)
{
    Poly po;
    if (!wire->toPoly(&po.points, &po.numpts))
        return (false);

    bool ret = true;
    BBox BBp;
    po.computeBB(&BBp);
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *BBaoi = get_bb(i);
        if (BBp.intersect(BBaoi, false) && po.intersect(BBaoi, false)) {
            if (!mpx_clip || *BBaoi >= BBp)
                ret = mpx_channels[i]->write_wire(wire);
            else {
                Zlist *zl = po.toZlist();
                Ylist *yl = new Ylist(new Zlist(BBaoi));
                ret = (Zlist::zl_and(&zl, yl) == XIok);
                Ylist::destroy(yl);

                if (zl) {
                    PolyList *pl = Zlist::to_poly_list(zl);
                    for (PolyList *p = pl; p; p = p->next) {
                        if (p->po.is_rect()) {
                            BBox BBb(p->po.points);
                            ret = mpx_channels[i]->write_box(&BBb);
                        }
                        else
                            ret = mpx_channels[i]->write_poly(&p->po);
                        if (!ret)
                            break;
                    }
                    PolyList::destroy(pl);
                }
            }
            if (!ret)
                break;
        }
    }

    delete [] po.points;
    return (ret);
}


bool
mpx_flat_out::write_text(const Text *text)
{
    if (out_mode == Physical && FIO()->IsNoReadLabels())
        return (true);

    BBox lBB(0, 0, text->width, text->height);
    cTfmStack stk;
    stk.TSetTransformFromXform(text->xform, text->width, text->height);
    stk.TTranslate(text->x, text->y);
    Poly po(5, 0);
    stk.TBB(&lBB, &po.points);
    stk.TPop();

    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        const BBox *BBaoi = get_bb(i);
        if (bound_isect(BBaoi, &lBB, &po) &&
                (!mpx_clip || *BBaoi >= lBB)) {
            ret = mpx_channels[i]->write_text(text);
            if (!ret)
                break;
        }
    }
    delete [] po.points;
    return (ret);
}


bool
mpx_flat_out::write_sref(const Instance*)
{
    return (false);
}


bool
mpx_flat_out::write_endlib(const char *topcell_name)
{
    bool ret = true;
    for (int i = 0; i < mpx_nchannels; i++) {
        ret = mpx_channels[i]->write_endlib(topcell_name);
        if (!ret)
            break;
    }
    return (ret);
}
// End of mpx_flat_out functions.


// In parallel, write out the grid areas, output is flat.
//
// ptop
//   Top-level symref.
//
// blist
//   List of regions to process, grid is ignored if blist given.
//
// gridsize
//   The grid size, internal units.  Grid is square.
//
// bloatval
//   The areas will be bloated by this value, can be negative.
//
// maxdepth
//   Depth to flatten.
//
// flat_map
//   When flattening, add _N to name of top-level cell, so names are
//   unique in output file collection.
//
// prms->destination()
// Base name for output files:  bname_nx_ny.ext, if null, use
// top-level cell name.  Can have a ".gz" suffix to indicate compression
// for GDSII/CGX output.
//
// prms.scale()
//   Apply scale factor to data befroe processing.  The gridsize, bloatval,
//   and prms->regionBB() are coordinates in the output.
//
// prms->use_win(), prms->regionBB()
//   Region to write, if not set use entire top-level cell BB.
//
// prms->filetype()
//   Format of files to write: Fcif, Fcif, Fgds, Foas.
//
// prms->flatten()  IGNORED
//
// prms->clip()
//   Geometry will be clipped to region.
//
// prms->ecf_level()  IGNORED  
//   Empty cell filtering level.
//
OItype
cCHD::write_multi_flat(symref_t *ptop, const FIOcvtPrms *prms,
    const Blist *blist, unsigned int gridsize, int bloatval,
    unsigned int maxdepth, bool flat_map)
{
    // Local struct for cleanup and timing info.
    struct wmf_cleanup : public TimeDbg
    {
        wmf_cleanup() : TimeDbg("chd_write_multi_flat")
            {
                ctab = 0;
                itab = 0;
                in = 0;
                channels = 0;
                out = 0;
                bnds = 0;
                nvals = 0;
            }

        ~wmf_cleanup()
            {
                delete ctab;
                delete itab;
                delete in;
                if (channels) {
                    for (int i = 0; i < nvals; i++)
                        delete channels[i];
                    delete [] channels;
                }
                delete out;
                delete [] bnds;
            }
            
        cCVtab          *ctab;
        chd_intab       *itab;
        cv_in           *in;
        cv_out          **channels;
        mpx_flat_out    *out;
        BBox            *bnds;
        int             nvals;
    };
    wmf_cleanup wmc;

    if (!ptop) {
        Errs()->add_error("write_multi_flat: unresolved top_cell");
        return (OIerror);
    }

    BBox aoiBB = prms->use_window() ? *prms->window() : *ptop->get_bb();
    if (prms->scale() != 1.0 && !prms->use_window())
        aoiBB.scale(prms->scale());

    //
    // Build table of cells to write.
    //
    bool ok = true;
    wmc.ctab = new cCVtab(false, 1);
    if (prms->use_window()) {
        BBox tBB(*prms->window());
        tBB.scale(1.0/prms->scale());
        if (!wmc.ctab->build_BB_table(this, ptop, 0, &tBB)) {
            Errs()->add_error(
                "write_multi_flat: failed to build cell table.");
            return (OIerror);
        }
        ok = wmc.ctab->build_TS_table(ptop, 0, maxdepth);
    }
    else {
        if (!wmc.ctab->build_BB_table(this, ptop, 0, 0)) {
            Errs()->add_error(
                "write_multi_flat: failed to build cell table.");
            return (OIerror);
        }
        ok = wmc.ctab->build_TS_table(ptop, 0, maxdepth);
    }
    if (!ok) {
        Errs()->add_error(
            "write_multi_flat: failed to build flat cell map table.");
        return (OIerror);
    }

    //
    // Set up the output channels.
    //
    const char *bname = prms->destination();
    if (!bname)
        bname = ptop->get_name()->string();
    char *fname = new char[strlen(bname) + 32];
    strcpy(fname, bname);

    bool do_gz = false;
    char *t = strrchr(fname, '.');
    if (t && lstring::cieq(t, ".gz")) {
        if (prms->filetype() == Fgds || prms->filetype() == Fcgx)
            do_gz = true;
        *t = 0;
    }
    char *fend = fname + strlen(fname);
    const char *ext = FIO()->GetTypeExt(prms->filetype());

    if (blist) {
        int cnt = 0;
        for (const Blist *bl = blist; bl; bl = bl->next) {
            if (bl->BB == CDnullBB)
                continue;
             cnt++;
        }

        wmc.nvals = cnt;
        wmc.channels = new cv_out*[wmc.nvals];
        memset(wmc.channels, 0, wmc.nvals*sizeof(cv_out*));
        wmc.bnds = new BBox[wmc.nvals];

        cnt = 0;
        int chcnt = 0;
        for (const Blist *bl = blist; bl; bl = bl->next) {
            cnt++;
            if (bl->BB == CDnullBB)
                continue;
            BBox gBB(bl->BB);
            gBB.bloat(bloatval);
            wmc.bnds[chcnt] = gBB;

            sprintf(fend, "%d%s", cnt, ext);
            if (do_gz)
                strcat(fend, ".gz");

            wmc.channels[chcnt] =
                FIO()->NewOutput(c_filename, fname, prms->filetype());
            if (!wmc.channels[chcnt])
                return (OIerror);

            // Apply back-end aliasing.
            //
            unsigned int mask = prms->alias_mask();
            if (prms->filetype() == Fgds)
                mask |= CVAL_GDS;
            FIOaliasTab *out_alias = FIO()->NewWritingAlias(mask, flat_map);
            wmc.channels[chcnt]->assign_alias(out_alias);
            if (flat_map) {
                const char *cn = ptop->get_name()->string();
                char *tbuf = new char[strlen(cn) + 32];
                sprintf(tbuf, "%s_%d", cn, cnt);
                out_alias->set_alias(cn, tbuf);
                delete [] tbuf;
            }
            wmc.channels[chcnt]->read_alias(fname);
            chcnt++;
        }
    }
    else {
        int nxc = aoiBB.width()/gridsize + (aoiBB.width()%gridsize != 0);
        int nyc = aoiBB.height()/gridsize + (aoiBB.height()%gridsize != 0);
        wmc.nvals = nxc*nyc;
        wmc.channels = new cv_out*[wmc.nvals];
        memset(wmc.channels, 0, wmc.nvals*sizeof(cv_out*));
        wmc.bnds = new BBox[wmc.nvals];

        int cnt = 0;
        for (int ic = 0; ic < nyc; ic++) {
            for (int jc = 0; jc < nxc; jc++) {
                int cx = aoiBB.left + jc*gridsize;
                int cy = aoiBB.bottom + ic*gridsize;
                BBox gBB(cx, cy, cx + gridsize, cy + gridsize);
                if (gBB.right > aoiBB.right)
                    gBB.right = aoiBB.right;
                if (gBB.top > aoiBB.top)
                    gBB.top = aoiBB.top;
                gBB.bloat(bloatval);
                wmc.bnds[cnt] = gBB;

                sprintf(fend, "_%d_%d%s", jc, ic, ext);
                if (do_gz)
                    strcat(fend, ".gz");

                wmc.channels[cnt] =
                    FIO()->NewOutput(c_filename, fname, prms->filetype());
                if (!wmc.channels[cnt])
                    return (OIerror);

                // Apply back-end aliasing.
                //
                unsigned int mask = prms->alias_mask();
                if (prms->filetype() == Fgds)
                    mask |= CVAL_GDS;
                FIOaliasTab *out_alias = FIO()->NewWritingAlias(mask,
                    flat_map);
                wmc.channels[cnt]->assign_alias(out_alias);
                if (flat_map) {
                    const char *cn = ptop->get_name()->string();
                    char *tbuf = new char[strlen(cn) + 32];
                    sprintf(tbuf, "%s_%d", cn, cnt);
                    out_alias->set_alias(cn, tbuf);
                    delete [] tbuf;
                }
                wmc.channels[cnt]->read_alias(fname);
                cnt++;
            }
        }
    }
    delete [] fname;

    //
    // Set up the input reader.
    //
    wmc.in = newInput(prms->allow_layer_mapping());
    if (!wmc.in) {
        Errs()->add_error("write_multi_flat: NewInput failed.");
        return (OIerror);
    }
    if (prms->use_window()) {
        wmc.in->set_area_filt(true, prms->window());
        wmc.in->set_clip(prms->clip());
    }

    //
    // Set input aliasing to CHD aliasing.
    //
    wmc.in->assign_alias(new FIOaliasTab(true, false, c_alias_info));

    //
    // Set up flattening.
    //
    if (maxdepth > CDMAXCALLDEPTH)
        maxdepth = CDMAXCALLDEPTH;
    wmc.in->set_flatten(maxdepth, cvTfFlatten);
    wmc.in->TPush();
    wmc.in->TLoad(CDtfRegI2);

    //
    // Set up additional CHD input channels, for library references.
    //
    wmc.itab = new chd_intab;
    for (int i = 0; ; i++) {
        cCHD *tchd = wmc.ctab->get_chd(i);
        if (!tchd)
            break;
        if (tchd == this) {
            wmc.itab->insert(tchd, wmc.in);
            wmc.itab->set_no_free(wmc.in);
            continue;
        }
        cv_in *new_in = tchd->newInput(prms->allow_layer_mapping());
        if (!new_in) {
            Errs()->add_error(
                "write_multi_flat: reference channel setup failed.");
            return (OIerror);
        }
        if (prms->use_window()) {
            new_in->set_area_filt(true, prms->window());
            new_in->set_clip(prms->clip());
        }
        new_in->set_flatten(maxdepth, cvTfFlatten);
        new_in->TPush();
        new_in->TLoad(CDtfRegI2);
        wmc.itab->insert(tchd, new_in);
    }

    //
    // Set up the back end.
    //
    wmc.out = new mpx_flat_out(wmc.nvals, wmc.bnds, wmc.channels,
        prms->clip());
    wmc.in->setup_backend(wmc.out);

    //
    // Do the work.
    //
    cv_in *cin = wmc.in;
    cCHD *cchd = this;
    CVtabGen cfgen(wmc.ctab, 0, ptop);

    if (!cin->chd_setup(cchd, wmc.ctab, 0, Physical, prms->scale())) {
        Errs()->add_error(
            "write_multi_flat: main channel setup failed.");
        ok = false;
    }
    if (ok && wmc.out) {
        if (!wmc.out->open_library(Physical, 1.0)) {
            Errs()->add_error(
                "write_multi_flat: main channel header write failed.");
            ok = false;
        }
    }

    if (ok) {
        bool first = true;
        cin->set_ignore_instances(true);
        unsigned char *tstream;
//        flat_read_fb frfb(ftab->num_cells());
        symref_t *tp;
        cCHD *tchd;
        cvtab_item_t *item;
        while ((item = cfgen.next()) != 0) {
            tchd = wmc.ctab->get_chd(item->get_chd_tkt());
            tp = item->symref();
            tstream = wmc.ctab->get_tstream(item);
            if (!tstream)
                continue;
            if (tchd != cchd) {
                cin->set_ignore_instances(false);
                cin->set_transform_level(0);
                cin->chd_finalize();
                FIOaliasTab *at = cin->extract_alias();
                cin = wmc.itab->find(tchd);
                cin->assign_alias(at);
                cchd = tchd;

                if (!cin->chd_setup(cchd, wmc.ctab, 0, Physical,
                        prms->scale())) {
                    Errs()->add_error(
                        "write_multi_flat: reference channel setup failed.");
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
//            frfb.check();
            cin->set_tf_list(tstream);
            ok = cin->chd_read_cell(tp, false);
            cin->set_tf_list(0);
            if (!ok) {
                Errs()->add_error(
                    "write_multi_flat: cell read failed.");
                break;
            }
        }
//        frfb.final();
    }
    cin->set_ignore_instances(false);
    cin->set_transform_level(0);
    cin->set_tf_list(0);
    cin->chd_finalize();
    wmc.in->assign_alias(cin->extract_alias());

    if (ok && wmc.out) {
        if (!wmc.out->write_end_struct()) {
            Errs()->add_error(
                "write_multi_flat: write end struct failed.");
            ok = false;
        }
        if (ok) {
            if (!wmc.in->no_end_lib() &&
                    !wmc.out->write_endlib(ptop->get_name()->string())) {
                Errs()->add_error(
                    "write_multi_flat: write end lib failed.");
                ok = false;
            }
        }
    }
    OItype oiret =
        ok ? OIok : cin->was_interrupted() ? OIaborted : OIerror;

    //
    // Write a native cell that calls all of the pieces.
    //
    if (oiret == OIok && flat_map) {
        stringlist *s0 = 0, *se = 0;
        for (int i = 0; i < wmc.nvals; i++) {
            stringlist *s =
                new stringlist(lstring::copy(wmc.channels[i]->filename()), 0);
            if (!s0)
                s0 = se = s;
            else {
                se->next = s;
                se = s;
            }
        }
        write_native_composite(bname, s0);
        s0->free();
    }

    return (oiret);
}

