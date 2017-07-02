
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
 $Id: ext_net_dump.cc,v 5.40 2016/05/14 20:29:39 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "cvrt.h"
#include "edit.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_net_dump.h"
#include "ext_grpgen.h"
#include "geo_ylist.h"
#include "cd_celldb.h"
#include "cd_lgen.h"
#include "cd_chkintr.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_chd_flat.h"
#include "fio_oasis.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "tech_layer.h"
#include "tech_via.h"

#include <algorithm>

//
// Code for net batch extraction.  This will generate an OASIS output
// file, where the "subcells" each contain a net (metal only), instead
// of the original hierarchy.  The algorithm is designed to be as memory
// efficient as possible, using a gridded approach with CHDs.
//


cExtNets::cExtNets(cCHD *chd, const char *cname, const char *basename,
    unsigned int flags)
{
    en_chd = chd;
    en_cellname = 0;
    en_basename = lstring::copy(basename);
    en_nx = 0;
    en_ny = 0;
    en_flags = flags;
    en_netcnt = 0;

    if (en_chd) {
        symref_t *p = en_chd->findSymref(cname, Physical, true);
        if (p) {
            en_cellname = p->get_name()->string();
            en_chd->setBoundaries(p);
            if (!en_basename)
                en_basename = lstring::copy(en_cellname);
        }
    }
}


// Stage 1
//
// For the grid regions, write out an OASIS nets file, and four
// edge-stitching files.  The OASIS file contains a top-level cell
// with the same name as the "real" top-level cell.  This contains in
// instance of each net cell, i.e., each net is in a separate subcell,
// containing metal only.  The original hierarchy is flattetned.
//
// The edge files are text files containing sorted records of the nets
// which touch the cell boundary.


bool
cExtNets::dump_nets_grid(int gridsize)
{
    if (!en_chd) {
        Errs()->add_error("dump_nets_grid: null CHD.");
        return (false);
    }
    if (!en_cellname) {
        Errs()->add_error("dump_nets_grid: null cellname, setup failed.");
        return (false);
    }
    if (!en_basename) {
        Errs()->add_error("dump_nets_grid: setup failed, basename missing.");
        return (false);
    }
    symref_t *p = en_chd->findSymref(en_cellname, Physical, false);
    if (!p) {
        Errs()->add_error("dump_nets_grid: unresolved cellname %s.",
            en_cellname);
        return (false);
    }

    BBox BB = *p->get_bb();
    en_nx = BB.width()/gridsize + (BB.width()%gridsize != 0);
    en_ny = BB.height()/gridsize + (BB.height()%gridsize != 0);
    int nvals = en_nx*en_ny;

    bool ret = true;
    FIO()->ifSetWorking(true);
    for (int ic = 0; ic < en_ny; ic++) {
        for (int jc = 0; jc < en_nx; jc++) {
            int cx = BB.left + jc*gridsize;
            int cy = BB.bottom + ic*gridsize;
            BBox cBB(cx, cy, cx + gridsize, cy + gridsize);
            if (cBB.right > BB.right)
                cBB.right = BB.right;
            if (cBB.top > BB.top)
                cBB.top = BB.top;

            if (!dump_nets(&cBB, jc, ic)) {
                Errs()->add_error("dump_nets_grid: net dump %d,%ds failed.",
                    jc, ic);
                ret = false;
                break;
            }

            FIO()->ifInfoMessage(IFMSG_INFO, "Completed %d of %d.",
                ic*en_nx + jc + 1, nvals);

            if (checkInterrupt()) {
                ret = false;
                break;
            }
        }
        if (!ret)
            break;
    }

    if (ret && !(en_flags & EN_STP1))
        ret = stage2();
    if (ret && !(en_flags & (EN_STP1 | EN_STP2)))
        ret = stage3();

    FIO()->ifSetWorking(false);
    return (ret);
}


bool
cExtNets::dump_nets(const BBox *AOI, int x, int y)
{
    if (!en_chd) {
        Errs()->add_error("dump_nets: null CHD.");
        return (false);
    }
    if (!en_cellname) {
        Errs()->add_error("dump_nets: null cellname, setup failed.");
        return (false);
    }
    if (!en_basename) {
        Errs()->add_error("dump_nets_grid: setup failed, basename missing.");
        return (false);
    }

    symref_t *p = en_chd->findSymref(en_cellname, Physical, false);
    if (!p) {
        Errs()->add_error("dump_nets: unresolved cellname %s.", en_cellname);
        return (false);
    }

    FIOcvtPrms prms;

    // Setup area filtering and flattening, with clipping.
    prms.set_use_window(true);
    prms.set_window(AOI);
    prms.set_clip(true);
    prms.set_flatten(true);
    prms.set_allow_layer_mapping(true);

    // Setup layer filtering.
    // First, set up a table if via tree layers, if necessary.
    SymTab *ltab = 0;
    if ((en_flags & EN_VIAS) && (en_flags & EN_VTRE) &&
            !(en_flags & EN_LFLT) && !(en_flags & EN_EXTR)) {
        CDl *ld;
        CDextLgen gen(CDL_VIA);
        while ((ld = gen.next()) != 0) {
            for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {
                if (!via->tree())
                    continue;
                CDll *l0 = via->tree()->findLayersInTree();
                for (CDll *l = l0; l; l = l->next) {
                    CDl *ldtmp = l->ldesc;
                    if (ldtmp->isVia() || ldtmp->isConductor())
                        continue;
                    // Keep only layers not found otherwise.
                    if (!ltab)
                        ltab = new SymTab(false, false);
                    if (ltab->get((unsigned long)ldtmp) == ST_NIL)
                        ltab->add((unsigned long)ldtmp, 0, false);
                }
                CDll::destroy(l0);
            }
        }
    }

    // Compose the layer list string.  Identify inverted ground plane
    // while traversing.
    sLstr lstr;
    CDlgen gen(Physical);
    CDl *ld;
    CDl *ldgp = 0;
    while ((ld = gen.next()) != 0) {
        if (ld->isConductor() || ld->isVia()) {
            if (!(en_flags & EN_LFLT)) {
                if (lstr.string())
                    lstr.add_c(' ');
                lstr.add(ld->name());
            }
            if (ld->isGroundPlane() && ld->isDarkField())
                ldgp = ld;
        }
        else if (ltab && ltab->get((unsigned long)ld) != ST_NIL) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add(ld->name());
        }
    }
    delete ltab;

    char *ltmp = 0, *btmp = 0;
    if (!(en_flags & EN_LFLT)) {
        ltmp = lstring::copy(CDvdb()->getVariable(VA_LayerList));
        btmp = lstring::copy(CDvdb()->getVariable(VA_UseLayerList));
        CDvdb()->setVariable(VA_LayerList, lstr.string());
        if (en_flags & EN_EXTR)
            CDvdb()->clearVariable(VA_UseLayerList);
        else
            CDvdb()->setVariable(VA_UseLayerList, "");
        // Layer filtering is actually used only when not extracting. 
        // When extracting, we (probably) need all layers for device
        // recognition.
    }

    // Read flat into database.
    OItype oiret = en_chd->readFlat(en_cellname, &prms, 0, CDMAXCALLDEPTH);

    // Reset layer filtering.
    if (!(en_flags & EN_LFLT)) {
        CDvdb()->setVariable(VA_LayerList, ltmp);
        if (!btmp)
            CDvdb()->clearVariable(VA_UseLayerList);
        else
            CDvdb()->setVariable(VA_UseLayerList, btmp);
        delete [] ltmp;
        delete [] btmp;
    }

    if (oiret != OIok) {
        Errs()->add_error("dump_nets: flat read failed.");
        return (false);
    }

    // Extract nets in flat cell.
    CDs *sdesc = CDcdb()->findCell(en_cellname, Physical);
    if (!sdesc) {
        Errs()->add_error("dump_nets: internal error, cell desc not found.");
        return (false);
    }

    if (ldgp) {
        // This is an inverse-polarity ground-plane layer.  We will
        // invert this layer and temporarily turn off the DARKFIELD
        // flag.  The output will contain the inverted layer.

        char buf[32];
        sprintf(buf, "%s = !%s", ldgp->name(), ldgp->name());
        if (ED()->createLayer(sdesc, buf, 0, CLdefault) != XIok) {
            Errs()->add_error("dump_nets: ground plane inversion failed.");
            return (false);
        }
        ldgp->setDarkField(false);
    }

    bool ret;
    if (en_flags & EN_EXTR)
        ret = EX()->extract(sdesc);
    else
        ret = EX()->group(sdesc, CDMAXCALLDEPTH);

    if (ldgp)
        ldgp->setDarkField(true);

    if (!ret) {
        Errs()->add_error("dump_nets: grouping failed.");
        return (false);
    }

    // Write the group file.
    if (!write_metal_file(sdesc, x, y))
        return (false);

    // Write the edge mapping file.
    if (!write_edge_map(sdesc, AOI, x, y))
        return (false);

    // clear database
    delete sdesc;
    return (true);
}


namespace {
    struct comp_setup
    {
        comp_setup() { t1 = t2 = false; t3 = 0; }
        ~comp_setup() { delete [] t3; }

        void set();
        void clear();

    private:
        bool t1;
        bool t2;
        char *t3;
    };

    void
    comp_setup::set()
    {
        t1 = CDvdb()->getVariable(VA_OasWriteCompressed);
        t2 = CDvdb()->getVariable(VA_OasWriteNameTab);
        t3 = lstring::copy(CDvdb()->getVariable(VA_OasWriteRep));
        CDvdb()->setVariable(VA_OasWriteCompressed, "");
        CDvdb()->setVariable(VA_OasWriteNameTab, "");
        CDvdb()->setVariable(VA_OasWriteRep, "");
    }

    void
    comp_setup::clear()
    {
        if (!t1)
            CDvdb()->clearVariable(VA_OasWriteCompressed);
        if (!t2)
            CDvdb()->clearVariable(VA_OasWriteNameTab);
        if (!t3)
            CDvdb()->clearVariable(VA_OasWriteRep);
        else
            CDvdb()->setVariable(VA_OasWriteRep, t3);
    }
}


// Write an OASIS file containing a top container cell (with the
// original top cell name) and a cell for each extracted wire net
// which is instantiated in the top cell.  The net cells are named
// "x_y_g" where x,y,g are integers.  The x,y are the grid
// coordinates, and g is the group number.  The file is named
// "basename_x_y.oas" where basename is the string passed to
// dump_nets_grid.
//
bool
cExtNets::write_metal_file(const CDs *sdesc, int x, int y) const
{
    if (!sdesc) {
        Errs()->add_error("write_metal_file: null cell descriptor.");
        return (false);
    }
    cGroupDesc *gd = sdesc->groups();
    if (!gd) {
        Errs()->add_error("write_metal_file: cell %s has no group info.",
            sdesc->cellname()->string());
        return (false);
    }

    char *outfile = new char[strlen(en_basename) + 40];
    sprintf(outfile, "%s_%d_%d.oas", en_basename, x, y);
    GCarray<char*> gc_outfile(outfile);

    comp_setup cs;
    if (en_flags & EN_COMP)
        cs.set();
    oas_out *oas = new oas_out(0);
    if (en_flags & EN_COMP)
        cs.clear();

    GCobject<oas_out*> gc_oas(oas);
    if (!oas->set_destination(outfile)) {
        Errs()->add_error("write_metal_file: can't open destination file %s.",
            outfile);
        return (false);
    }

    if (!oas->write_begin(1.0)) {
        Errs()->add_error("write_metal_file: write_begin returned error.");
        return (false);
    }

    char cname[64];
    sprintf(cname, "%d_%d", x, y);
    char *cptr = cname + strlen(cname);

    for (int i = 0; i < gd->num_groups(); i++) {
        const sGroup *grp = gd->group_for(i);
        if (!grp->net())
            continue;
        sprintf(cptr, "_%d", i);
        // Avoid writing empty cells.
        if (!oas->write_begin_struct(cname)) {
            Errs()->add_error(
            "write_metal_file: write_begin_struct %s returned error.",
                cname);
            return (false);
        }
        for (CDol *ol = grp->net()->objlist(); ol; ol = ol->next) {
            if (!oas->write_object(ol->odesc, 0)) {
                Errs()->add_error("write_metal_file: write_object failed.");
                return (false);
            }
        }

        if (en_flags & EN_VIAS) {
            if (!write_vias(sdesc, grp, oas)) {
                Errs()->add_error("write_metal_file: write_vias failed.");
                return (false);
            }
        }

        if (!oas->write_end_struct()) {
            Errs()->add_error(
                "write_metal_file: write_end_struct %s returned error.",
                cname);
            return (false);
        }
    }
    if (!oas->write_begin_struct(sdesc->cellname()->string())) {
        Errs()->add_error(
            "write_metal_file: write_begin_struct %s returned error.",
            sdesc->cellname()->string());
        return (false);
    }
    Instance inst;
    for (int i = 0; i < gd->num_groups(); i++) {
        const sGroup *grp = gd->group_for(i);
        if (!grp->net())
            continue;
        sprintf(cptr, "_%d", i);
        inst.name = cname;
        if (!oas->write_sref(&inst)) {
            Errs()->add_error(
                "write_metal_file: write_sref %s returned error.",
                cname);
            return (false);
        }
    }
    if (!oas->write_end_struct(sdesc->cellname()->string())) {
        Errs()->add_error(
            "write_metal_file: write_end_struct %s returned error.",
            sdesc->cellname()->string());
        return (false);
    }
    if (!oas->write_endlib(sdesc->cellname()->string())) {
        Errs()->add_error(
            "write_metal_file: write_endlib returned error.");
        return (false);
    }
    return (true);
}


bool
cExtNets::write_vias(const CDs *sdesc, const sGroup *grp, oas_out *oas) const
{
    if (!grp->net())
        return (true);

    // Create a hash table of Zlists keyed by layer desc.
    //
    SymTab *tab = new SymTab(false, false);
    for (const CDol *ol = grp->net()->objlist(); ol; ol = ol->next) {
        Zlist *zl = ol->odesc->toZlist();
        if (!zl)
            continue;
        SymTabEnt *h = tab->get_ent((unsigned long)ol->odesc->ldesc());
        if (!h) {
            tab->add((unsigned long)ol->odesc->ldesc(), 0, false);
            h = tab->get_ent((unsigned long)ol->odesc->ldesc());
        }
        if (!h->stData)
            h->stData = zl;
        else {
            Zlist *zx = zl;
            while (zx->next)
                zx = zx->next;
            zx->next = (Zlist*)h->stData;
            h->stData = zl;
        }
    }

    // Clip/merge the conductor lists.
    //
    {
        SymTabGen stgen(tab);
        SymTabEnt *h;
        while ((h = stgen.next()) != 0)
            h->stData = Zlist::repartition_ni(((Zlist*)h->stData));
    }

    // Cycle through VIA layers.  For each via, find and save
    // via_layer & c1_layer & c2_layer.
    //
    SymTab *out_tab = new SymTab(false, false);
    XIrt ret = XIok;
    CDl *ld;
    CDextLgen lgen(CDL_VIA, CDextLgen::TopToBot);
    while ((ld = lgen.next()) != 0) {
        Zlist *zv0 = 0;
        for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {
            CDl *ld1 = via->layer1();
            CDl *ld2 = via->layer2();
            if (!ld1 || !ld2)
                continue;

            Zlist *z1 = (Zlist*)tab->get((unsigned long)ld1);
            if (z1 == (Zlist*)ST_NIL)
                continue;
            Zlist *z2 = (Zlist*)tab->get((unsigned long)ld2);
            if (z2 == (Zlist*)ST_NIL)
                continue;
            z1 = Zlist::copy(z1);
            z2 = Zlist::copy(z2);

            ret = Zlist::zl_and(&z1, z2);
            if (ret != XIok) {
                if (ret == XIbad)
                    Errs()->add_error(
                        "write_vias: clipping function returned error");
                break;
            }
            if (!z1)
                continue;

            Zlist *zv = sdesc->getZlist(0, ld, z1, &ret);
            Zlist::destroy(z1);
            if (ret != XIok) {
                if (ret == XIbad)
                    Errs()->add_error("write_vias: failed to get zlist for %s",
                        ld->name());
                break;
            }
            if (zv) {
                bool istrue = !via->tree();
                if (!istrue) {
                    sLspec lsp;
                    lsp.set_tree(via->tree());
                    ret = lsp.testContact(sdesc, 0, zv, &istrue);
                    lsp.set_tree(0);
                    if (ret != XIok) {
                        if (ret == XIbad)
                            Errs()->add_error(
                                "write_vias: via check returned error");
                        Zlist::destroy(zv);
                        break;
                    }
                    if (istrue && (en_flags & EN_VTRE)) {
                        CDll *l0 = via->tree()->findLayersInTree();
                        for (CDll *l = l0; l; l = l->next) {
                            CDl *ldtmp = l->ldesc;
                            Zlist *zx = sdesc->getZlist(0, ldtmp, zv, &ret);
                            if (ret != XIok) {
                                if (ret == XIbad)
                                    Errs()->add_error(
                                    "write_vias: failed to get zlist for %s",
                                        ldtmp->name());
                                Zlist::destroy(zv);
                                break;
                            }
                            if (zx) {
                                PolyList *pl = Zlist::to_poly_list(zx);
                                CDo *od = PolyList::to_odesc(pl, ldtmp);

                                if (od) {
                                    SymTabEnt *h =
                                        out_tab->get_ent((unsigned long)ldtmp);
                                    if (!h)
                                        out_tab->add((unsigned long)ldtmp, od,
                                            false);
                                    else {
                                        CDo *ox = od;
                                        while (ox->next_odesc())
                                            ox = ox->next_odesc();
                                        ox->set_next_odesc((CDo*)h->stData);
                                        h->stData = od;
                                    }
                                }
                            }
                        }
                        CDll::destroy(l0);
                        if (ret != XIok)
                            break;
                    }
                }
                if (istrue) {
                    Zlist *zvt = zv;
                    while (zvt->next)
                        zvt = zvt->next;
                    zvt->next = zv0;
                    zv0 = zv;
                }
                else
                    Zlist::destroy(zv);
            }
        }
        if (ret != XIok) {
            Zlist::destroy(zv0);
            break;
        }

        // Subtlety here: the clip/merge in toPolyList should eliminate
        // coincident via objects generated from multiple Via lines on
        // a layer.

        PolyList *po = Zlist::to_poly_list(zv0);
        CDo *od = PolyList::to_odesc(po, ld);

        if (od) {
            SymTabEnt *h = out_tab->get_ent((unsigned long)ld);
            if (!h)
                out_tab->add((unsigned long)ld, od, false);
            else {
                CDo *ox = od;
                while (ox->next_odesc())
                    ox = ox->next_odesc();
                ox->set_next_odesc((CDo*)h->stData);
                h->stData = od;
            }
        }
    }

    // Clear the zoid table.
    //
    SymTabGen gen(tab, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        Zlist::destroy(((Zlist*)h->stData));
        delete h;
    }
    delete tab;

    // Write/clear the object table.
    SymTabGen ogen(out_tab, true);
    while ((h = ogen.next()) != 0) {
        CDo *on;
        for (CDo *od = (CDo*)h->stData; od; od = on) {
            on = od->next_odesc();
            if (ret == XIok && !oas->write_object(od, 0)) {
                Errs()->add_error("write_vias: write_object failed.");
                ret = XIbad;
            }
            delete od;
        }
        delete h;
    }
    delete out_tab;
    return (ret == XIok);
}


#define EDG_MAGIC "Edge Mapping Info"
#define EDG_BEGIN "# BEGIN DATA"
#define EDG_END   "# END DATA"

// Write the (at most) four edge-mapping files, one for each edge. 
// The format is:
//
// Edge Mapping Info
// TopCell: <cellname>
// # BEGIN DATA
// L|B|R|T start end group layer_name
// ...
// # END DATA
//
// The first line is literal and must appear as shown.  The records
// start with an edge designation character.  The start and end are
// values in microns, relative to the coordinates of the original
// top-level cell.  These ranges are dark for the the given layer. 
// The records are printed grouped by layers, ascending in the start
// value.
//
// The files are named
//   basename_x_y_L.edg
//   basename_x_y_B.edg
//   basename_x_y_R.edg
//   basename_x_y_T.edg
// where x,y are the grid indices.
//
bool
cExtNets::write_edge_map(const CDs *sdesc, const BBox *AOI, int x, int y) const
{
    if (!sdesc) {
        Errs()->add_error("write_edge_map: null cell descriptor.");
        return (false);
    }
    cGroupDesc *gd = sdesc->groups();
    if (!gd) {
        Errs()->add_error("write_edge_map: cell %s has no group info.",
            sdesc->cellname()->string());
        return (false);
    }

    char *filename = new char[strlen(en_basename) + 40];
    sprintf(filename, "%s_%d_%d", en_basename, x, y);
    char *fptr = filename + strlen(filename);
    GCarray<char*> gc_filename(filename);

    BBox BB(*AOI);
    emrec_t *em0 = 0;

    // Left edge.
    if (x > 0) {
        BB.right = BB.left + 10;
        CDl *ld;
        CDextLgen gen(CDL_CONDUCTOR);
        while ((ld = gen.next()) != 0) {
            sGrpGen gdesc;
            gdesc.init_gen(gd, ld, &BB);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->type() == CDBOX) {
                    if (odesc->oBB().left == AOI->left)
                        em0 = new emrec_t(odesc->ldesc()->name(),
                            odesc->oBB().bottom, odesc->oBB().top,
                            odesc->group(), em0);
                }
                else if (odesc->type() == CDPOLYGON) {
                    const Point *pts = ((CDpo*)odesc)->points();
                    int numpts = ((CDpo*)odesc)->numpts();
                    for (int i = 1; i < numpts; i++) {
                        if (pts[i].x == AOI->left &&
                                pts[i-1].x == AOI->left) {
                            if (pts[i].y > pts[i-1].y)
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i-1].y, pts[i].y, odesc->group(), em0);
                            else
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i].y, pts[i-1].y, odesc->group(), em0);
                        }
                    }
                }
                else if (odesc->type() == CDWIRE) {
                    // Wires that cross the boundary have been converted
                    // to polygons.
                    Poly wp;
                    if (!((CDw*)odesc)->w_toPoly(&wp.points, &wp.numpts))
                        continue;
                    const Point *pts = wp.points;
                    for (int i = 1; i < wp.numpts; i++) {
                        if (pts[i].x == AOI->left &&
                                pts[i-1].x == AOI->left) {
                            if (pts[i].y > pts[i-1].y)
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i-1].y, pts[i].y, odesc->group(), em0);
                            else
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i].y, pts[i-1].y, odesc->group(), em0);
                        }
                    }
                    delete [] pts;
                }
            }
        }
        BB.right = AOI->right;
        strcpy(fptr, "_L.edg");
        if (em0) {
            FILE *fp = large_fopen(filename, "w");
            if (!fp) {
                Errs()->add_error("write_edge_map: failed to open file %s.",
                    filename); 
                return (false);
            }
            fprintf(fp, "%s\n", EDG_MAGIC);
            fprintf(fp, "TopCell: %s\n", en_cellname);
            fprintf(fp, "%s\n", EDG_BEGIN);
            em0 = em0->sort_edge();
            while (em0) {
                em0->print(fp, 'L');
                emrec_t *et = em0;
                em0 = em0->next_rec();
                delete et;
            }
            fprintf(fp, "%s\n", EDG_END);
            fclose(fp);
        }
        else
            unlink(filename);
    }

    // Bottom edge.
    if (y > 0) {
        BB.top = BB.bottom + 10;
        CDl *ld;
        CDextLgen gen(CDL_CONDUCTOR);
        while ((ld = gen.next()) != 0) {
            sGrpGen gdesc;
            gdesc.init_gen(gd, ld, &BB);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->type() == CDBOX) {
                    if (odesc->oBB().bottom == AOI->bottom)
                        em0 = new emrec_t(odesc->ldesc()->name(),
                            odesc->oBB().left, odesc->oBB().right,
                            odesc->group(), em0);
                }
                else if (odesc->type() == CDPOLYGON) {
                    const Point *pts = ((CDpo*)odesc)->points();
                    int numpts = ((CDpo*)odesc)->numpts();
                    for (int i = 1; i < numpts; i++) {
                        if (pts[i].y == AOI->bottom &&
                                pts[i-1].y == AOI->bottom) {
                            if (pts[i].x > pts[i-1].x)
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i-1].x, pts[i].x, odesc->group(), em0);
                            else
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i].x, pts[i-1].x, odesc->group(), em0);
                        }
                    }
                }
                else if (odesc->type() == CDWIRE) {
                    // Wires that cross the boundary have been converted
                    // to polygons.
                    Poly wp;
                    if (!((CDw*)odesc)->w_toPoly(&wp.points, &wp.numpts))
                        continue;
                    const Point *pts = wp.points;
                    for (int i = 1; i < wp.numpts; i++) {
                        if (pts[i].y == AOI->bottom &&
                                pts[i-1].y == AOI->bottom) {
                            if (pts[i].x > pts[i-1].x)
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i-1].x, pts[i].x, odesc->group(), em0);
                            else
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i].x, pts[i-1].x, odesc->group(), em0);
                        }
                    }
                    delete [] pts;
                }
            }
        }
        BB.top = AOI->top;
        strcpy(fptr, "_B.edg");
        if (em0) {
            FILE *fp = large_fopen(filename, "w");
            if (!fp) {
                Errs()->add_error("write_edge_map: failed to open file %s.",
                    filename); 
                return (false);
            }
            fprintf(fp, "Edge Mapping Info\n");
            fprintf(fp, "TopCell: %s\n", en_cellname);
            fprintf(fp, "%s\n", EDG_BEGIN);
            em0 = em0->sort_edge();
            while (em0) {
                em0->print(fp, 'B');
                emrec_t *et = em0;
                em0 = em0->next_rec();
                delete et;
            }
            fprintf(fp, "%s\n", EDG_END);
            fclose(fp);
        }
        else
            unlink(filename);
    }

    // Right edge.
    if (x < en_nx-1) {
        BB.left = BB.right - 10;
        CDl *ld;
        CDextLgen gen(CDL_CONDUCTOR);
        while ((ld = gen.next()) != 0) {
            sGrpGen gdesc;
            gdesc.init_gen(gd, ld, &BB);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                // Only boxes and polygons, wires have been converted to
                // polys.
                if (odesc->type() == CDBOX) {
                    if (odesc->oBB().right == AOI->right)
                        em0 = new emrec_t(odesc->ldesc()->name(),
                            odesc->oBB().bottom, odesc->oBB().top,
                            odesc->group(), em0);
                }
                else if (odesc->type() == CDPOLYGON) {
                    const Point *pts = ((CDpo*)odesc)->points();
                    int numpts = ((CDpo*)odesc)->numpts();
                    for (int i = 1; i < numpts; i++) {
                        if (pts[i].x == AOI->right &&
                                pts[i-1].x == AOI->right) {
                            if (pts[i].y > pts[i-1].y)
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i-1].y, pts[i].y, odesc->group(), em0);
                            else
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i].y, pts[i-1].y, odesc->group(), em0);
                        }
                    }
                }
                else if (odesc->type() == CDWIRE) {
                    // Wires that cross the boundary have been converted
                    // to polygons.
                    Poly wp;
                    if (!((CDw*)odesc)->w_toPoly(&wp.points, &wp.numpts))
                        continue;
                    const Point *pts = wp.points;
                    for (int i = 1; i < wp.numpts; i++) {
                        if (pts[i].x == AOI->right &&
                                pts[i-1].x == AOI->right) {
                            if (pts[i].y > pts[i-1].y)
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i-1].y, pts[i].y, odesc->group(), em0);
                            else
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i].y, pts[i-1].y, odesc->group(), em0);
                        }
                    }
                    delete [] pts;
                }
            }
        }
        BB.left = AOI->left;
        strcpy(fptr, "_R.edg");
        if (em0) {
            FILE *fp = large_fopen(filename, "w");
            if (!fp) {
                Errs()->add_error("write_edge_map: failed to open file %s.",
                    filename); 
                return (false);
            }
            fprintf(fp, "Edge Mapping Info\n");
            fprintf(fp, "TopCell: %s\n", en_cellname);
            fprintf(fp, "%s\n", EDG_BEGIN);
            em0 = em0->sort_edge();
            while (em0) {
                em0->print(fp, 'R');
                emrec_t *et = em0;
                em0 = em0->next_rec();
                delete et;
            }
            fprintf(fp, "%s\n", EDG_END);
            fclose(fp);
        }
        else
            unlink(filename);
    }

    // Top edge.
    if (y < en_ny-1) {
        BB.bottom = BB.top - 10;
        CDl *ld;
        CDextLgen gen(CDL_CONDUCTOR);
        while ((ld = gen.next()) != 0) {
            sGrpGen gdesc;
            gdesc.init_gen(gd, ld, &BB);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                // Only boxes and polygons, wires have been converted to
                // polys.
                if (odesc->type() == CDBOX) {
                    if (odesc->oBB().top == AOI->top)
                        em0 = new emrec_t(odesc->ldesc()->name(),
                            odesc->oBB().left, odesc->oBB().right,
                            odesc->group(), em0);
                }
                else if (odesc->type() == CDPOLYGON) {
                    const Point *pts = ((CDpo*)odesc)->points();
                    int numpts = ((CDpo*)odesc)->numpts();
                    for (int i = 1; i < numpts; i++) {
                        if (pts[i].y == AOI->top &&
                                pts[i-1].y == AOI->top) {
                            if (pts[i].x > pts[i-1].x)
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i-1].x, pts[i].x, odesc->group(), em0);
                            else
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i].x, pts[i-1].x, odesc->group(), em0);
                        }
                    }
                }
                else if (odesc->type() == CDWIRE) {
                    // Wires that cross the boundary have been converted
                    // to polygons.
                    Poly wp;
                    if (!((CDw*)odesc)->w_toPoly(&wp.points, &wp.numpts))
                        continue;
                    const Point *pts = wp.points;
                    for (int i = 1; i < wp.numpts; i++) {
                        if (pts[i].y == AOI->top &&
                                pts[i-1].y == AOI->top) {
                            if (pts[i].x > pts[i-1].x)
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i-1].x, pts[i].x, odesc->group(), em0);
                            else
                                em0 = new emrec_t(odesc->ldesc()->name(),
                                    pts[i].x, pts[i-1].x, odesc->group(), em0);
                        }
                    }
                    delete [] pts;
                }
            }
        }
        BB.bottom = AOI->bottom;
        strcpy(fptr, "_T.edg");
        if (em0) {
            FILE *fp = large_fopen(filename, "w");
            if (!fp) {
                Errs()->add_error("write_edge_map: failed to open file %s.",
                    filename); 
                return (false);
            }
            fprintf(fp, "Edge Mapping Info\n");
            fprintf(fp, "TopCell: %s\n", en_cellname);
            fprintf(fp, "%s\n", EDG_BEGIN);
            em0 = em0->sort_edge();
            while (em0) {
                em0->print(fp, 'T');
                emrec_t *et = em0;
                em0 = em0->next_rec();
                delete et;
            }
            fprintf(fp, "%s\n", EDG_END);
            fclose(fp);
        }
        else
            unlink(filename);
    }

    return (true);
}


// Stage 2
//
// Looking at the edge files for adjacent grid regions, create an
// equivalence file which lists pairs of nets that should be connected
// across the boundary.

// Equivalence file format:
//
// Net Equivalences
// TopCell: <cellname>
// Base: <basename>
// # BEGIN DATA
// x1 y1 g1 x2 y2 g2 [layer overlap_min overlap_max]
// ...
// # END DATA

// The square-bracketed terms are optional debugging output.
// Once used, the edge files can be deleted.


#define EQV_MAGIC "Net Equivalences"
#define EQV_BEGIN "# BEGIN DATA"
#define EQV_END   "# END DATA"

namespace {
    void
    tab_destroy(SymTab *st)
    {
        SymTabGen gen(st, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            const char *name = h->stTag;
            emrec_t *em = (emrec_t*)h->stData;
            delete [] name;
            em->free();
            delete h;
        }
        delete st;
    }
}


bool
cExtNets::stage2()
{
    char *fn1 = new char[strlen(en_basename) + 40];
    char *pfn1 = lstring::stpcpy(fn1, en_basename);
    GCarray<char*> gc_fn1(fn1);

    char *fn2 = new char[strlen(en_basename) + 40];
    char *pfn2 = lstring::stpcpy(fn2, en_basename);
    GCarray<char*> gc_fn2(fn2);

    char *outname = new char[strlen(en_basename) + 8];
    sprintf(outname, "%s.equiv", en_basename);
    FILE *outfp = large_fopen(outname, "w");
    if (!outfp) {
        Errs()->add_error("stage2: can't open equiv file %s.", outname);
        delete [] outname;
        return (false);
    }
    delete [] outname;
    fprintf(outfp, "%s\n", EQV_MAGIC);
    fprintf(outfp, "TopCell: %s\n", en_cellname);
    fprintf(outfp, "Base: %s\n", en_basename);
    fprintf(outfp, "%s\n", EQV_BEGIN);

    // Read and parse the edge files.  Missing files are assumed to
    // indicate lack of connection points, not an error.

    for (int ic = 0; ic < en_ny; ic++) {
        for (int jc = 0; jc < en_nx; jc++) {
            if (jc + 1 < en_nx) {
                sprintf(pfn1, "_%d_%d_R.edg", jc, ic);
                FILE *fp1 = large_fopen(fn1, "r");

                sprintf(pfn2, "_%d_%d_L.edg", jc+1, ic);
                FILE *fp2 = large_fopen(fn2, "r");

                bool ret = true;
                if (fp1 && fp2) {
                    SymTab *st1;
                    ret = parse_edge_file(fp1, &st1);
                    if (!ret) {
                        tab_destroy(st1);
                        Errs()->add_error("stage2: parse error in %s.", fn1);
                    }
                    else {
                        SymTab *st2;
                        ret = parse_edge_file(fp2, &st2);
                        if (!ret) {
                            tab_destroy(st1);
                            tab_destroy(st2);
                            Errs()->add_error(
                                "stage2: parse error in %s.", fn1);
                        }
                        else {
                            ret = reduce(outfp, st1, st2, jc, ic, jc+1, ic);
                            tab_destroy(st1);
                            tab_destroy(st2);
                            if (!ret) {
                                Errs()->add_error(
                                    "stage2: reduce %s %s failed.", fn1, fn2);
                            }
                        }
                    }
                }
                if (fp1)
                    fclose(fp1);
                if (fp2)
                    fclose(fp2);
                if (!ret) {
                    fclose(outfp);
                    return (false);
                }
                if (!(en_flags & EN_KEEP)) {
                    unlink(fn1);
                    unlink(fn2);
                }
            }
            if (ic + 1 < en_ny) {
                sprintf(pfn1, "_%d_%d_T.edg", jc, ic);
                FILE *fp1 = large_fopen(fn1, "r");

                sprintf(pfn2, "_%d_%d_B.edg", jc, ic+1);
                FILE *fp2 = large_fopen(fn2, "r");

                bool ret = true;
                if (fp1 && fp2) {
                    SymTab *st1;
                    ret = parse_edge_file(fp1, &st1);
                    if (!ret) {
                        tab_destroy(st1);
                        Errs()->add_error("stage2: parse error in %s.", fn1);
                    }
                    else {
                        SymTab *st2;
                        ret = parse_edge_file(fp2, &st2);
                        if (!ret) {
                            tab_destroy(st1);
                            tab_destroy(st2);
                            Errs()->add_error(
                                "stage2: parse error in %s.", fn1);
                        }
                        else {
                            ret = reduce(outfp, st1, st2, jc, ic, jc, ic+1);
                            tab_destroy(st1);
                            tab_destroy(st2);
                            if (!ret) {
                                Errs()->add_error(
                                    "stage2: reduce %s %s failed.", fn1, fn2);
                            }
                        }
                    }
                }
                if (fp1)
                    fclose(fp1);
                if (fp2)
                    fclose(fp2);
                if (!ret) {
                    fclose(outfp);
                    return (false);
                }
                if (!(en_flags & EN_KEEP)) {
                    unlink(fn1);
                    unlink(fn2);
                }
            }
        }
    }
    fprintf(outfp, "%s\n", EQV_END);
    fclose(outfp);
    return (true);
}


bool
cExtNets::parse_edge_file(FILE *fp, SymTab **stret)
{
    *stret = 0;
    char buf[256];
    char *s = fgets(buf, 256, fp);
    if (!s || !lstring::prefix(EDG_MAGIC, s)) {
        Errs()->add_error(
            "parse_edge_file: bad magic, format not recognized.");
        return (false);
    }
    while ((s = fgets(buf, 256, fp)) != 0) {
        if (lstring::prefix(EDG_BEGIN, s))
            break;
    }
    if (!s) {
        Errs()->add_error("parse_edge_file: bad format.");
        return (false);
    }

    SymTab *tab = new SymTab(false, false);

    while ((s = fgets(buf, 256, fp)) != 0) {
        char *tok = lstring::gettok(&s);
        if (!tok)
            continue;
        if (strcmp(tok, "L") && strcmp(tok, "B") &&
                strcmp(tok, "R") && strcmp(tok, "T")) {
            delete [] tok;
            if (lstring::prefix(EDG_END, s))
                break;
            continue;
        }
        delete [] tok;
        char *st = lstring::gettok(&s);
        char *en = lstring::gettok(&s);
        char *gp = lstring::gettok(&s);
        char *ln = lstring::gettok(&s);

        if (!ln) {
            delete [] st;
            delete [] en;
            delete [] gp;
            continue;
        }
        int start = INTERNAL_UNITS(atof(st));
        int end = INTERNAL_UNITS(atof(en));
        int grp = atoi(gp);
        delete [] st;
        delete [] en;
        delete [] gp;

        SymTabEnt *h = tab->get_ent(ln);
        if (h) {
            h->stData = new emrec_t(h->stTag, start, end, grp,
                (emrec_t*)h->stData);
            delete [] ln;
        }
        else {
            emrec_t *em = new emrec_t(ln, start, end, grp, 0);
            tab->add(ln, em, false);
        }
    }

    // The list ordering must be re-reversed.  It was reversed when
    // read above.
    SymTabGen gen(tab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        emrec_t *em0 = 0;
        emrec_t *em = (emrec_t*)h->stData;
        while (em) {
            emrec_t *et = em;
            em = em->next_rec();
            et->set_next_rec(em0);
            em0 = et;
        }
        h->stData = em0;
    }

    *stret = tab;
    return (true);
}


bool
cExtNets::reduce(FILE *fp, SymTab *st1, SymTab *st2, int x1, int y1,
    int x2, int y2)
{
    // Remove terms from st1, remove similar term from st2 and compare.

    SymTabGen gen1(st1, true);
    SymTabEnt *h1;
    while ((h1 = gen1.next()) != 0) {
        const char *name1 = h1->stTag;
        emrec_t *em1 = (emrec_t*)h1->stData;
        delete h1;

        const char *name2 = 0;
        emrec_t *em2 = 0;
        SymTabEnt *h2 = st2->get_ent(name1);
        if (h2) {
            name2 = h2->stTag;
            em2 = (emrec_t*)h2->stData;
            st2->remove(name2);
        }
        if (em1 && em2) {
            if (!reduce(em1, em2, fp, x1, y1, x2, y2, name1)) {
                Errs()->add_error("reduce: reduction failed.");
                delete [] name1;
                delete [] name2;
                em1->free();
                em2->free();
                return (false);
            }
        }
        delete [] name1;
        delete [] name2;
        em1->free();
        em2->free();
    }

    return (true);
}


namespace {
    inline bool
    overlap(emrec_t *em1, emrec_t *em2)
    {
        if (em1->endval() < em2->startval() || em2->endval() < em1->startval())
            return (false);
        return (true);
    }
}


bool
cExtNets::reduce(emrec_t *em1, emrec_t *em2, FILE *fp, int x1, int y1,
    int x2, int y2, const char *lname)
{
    // The lists are sorted in ascending order in start.
    while (em1 && em2) {

        if (em1->endval() < em2->startval()) {
            em1 = em1->next_rec();
            continue;
        }
        if (em2->endval() < em1->startval()) {
            em2 = em2->next_rec();
            continue;
        }

        if (overlap(em1, em2)) {
            int min = mmMax(em1->startval(), em2->startval());
            int max = mmMin(em1->endval(), em2->endval());
            if (max > min) {
                if (en_flags & EN_EQVB) {
                    if (fprintf(fp, "%d %d %d %d %d %d   %s %d %d\n", x1, y1,
                            em1->grpval(), x2, y2, em2->grpval(), lname, min,
                            max) < 0) {
                        Errs()->add_error("reduce: write error.");
                        return (false);
                    }
                }
                else {
                    if (fprintf(fp, "%d %d %d %d %d %d\n", x1, y1,
                            em1->grpval(), x2, y2, em2->grpval()) < 0) {
                        Errs()->add_error("reduce: write error.");
                        return (false);
                    }
                }
            }
        }

        if (em1->endval() > em2->endval())
            em2 = em2->next_rec();
        else 
            em1 = em1->next_rec();
    }
    return (true);
}


// Stage 3:
//
// Open the equivalence file.  From this, create two tables in memory:
//
// a. Tag:  First id string in traverse order of grid cells.
//    Data: Table of remaining id strings in group.
// b. Tag:  Id string found in any group.
//    Data: Table containing this tag.
//
// Open new OASIS output file for writing.
//
// traverse grid cells
//     open grid nets file
//     traverse groups
//         if (group in a || group not in b) {
//             write group cell to output
//             if (group in a)
//                 add an instance of each connected group.
//                 or
//                 add geometry directly (flat mode)
//             end
//         end
//     end
// end
//
// The resulting OASIS file has a top-level cell with the same name as
// the original design.  There will be a subcell for every net.  For
// nets that extend across boundaries, the net cells will have
// subcells that reference the connected nets, or the geometry from
// the connected nets will be imported into the cell (flat mode).  In
// any case, these cells will contain the geometry of the "primary"
// net, which is the first net encountered in traversal ordering
// (group number, left to right, bottom to top).


namespace {
    struct stage3_cleanup
    {
        stage3_cleanup(SymTab *t) { tab = t; }
        ~stage3_cleanup();
    private:
        SymTab *tab;
    };

    stage3_cleanup::~stage3_cleanup()
    {
        SymTabGen gen(tab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            SymTab *st = (SymTab*)h->stData;
            delete st;
            delete h;
        }
        delete tab;
    }


    // Test if id1 is ahead of id2 in traversal order.  Returns are
    // -1  id1 before id2
    //  1  id1 after id2
    //  0  inconclusive
    //
    int
    test_order(const char *id1, const char *id2)
    {
        int x1, y1, g1;
        if (sscanf(id1, "%d_%d_%d", &x1, &y1, &g1) != 3)
            return (0);
        int x2, y2, g2;
        if (sscanf(id2, "%d_%d_%d", &x2, &y2, &g2) != 3)
            return (0);
        if (y1 < y2)
            return (-1);
        if (y1 > y2)
            return (1);
        if (x1 < x2)
            return (-1);
        if (x1 > x2)
            return (1);
        if (g1 < g2)
            return (-1);
        if (g1 > g2)
            return (1);
        return (0);
    }


    // Create input channel for this CHD.
    //
    cv_in *
    setup_input(cCHD *chd, oas_out *oas)
    {
        cv_in *in = chd->newInput(false);  // no layer mapping
        if (!in) {
            Errs()->add_error(
                "setup_input: main input channel creation failed.");
            return (0);
        }
        // Set input aliasing to CHD aliasing.
        in->assign_alias(new FIOaliasTab(true, false, chd->aliasInfo()));

        if (!in->setup_source(chd->filename())) {
            Errs()->add_error("setup_input: source setup failed.");
            delete in;
            return (0);
        }
        if (!in->setup_backend(oas)) {
            Errs()->add_error("setup_input: back end setup failed.");
            delete in;
            return (0);
        }
        in->backend();  // revert ownership of oas

        if (!in->chd_setup(chd, 0, 0, Physical, 1.0)) {
            Errs()->add_error("setup_input: main channel setup failed.");
            delete in;
            return (0);
        }
        return (in);
    }
}


bool
cExtNets::stage3()
{
    SymTab *st_a = new SymTab(false, false);
    // The st_a is tagged by the first-encountered id string of a group
    // in traversal order, the data are a table of other group ids.

    stage3_cleanup gc_st_a(st_a);

    SymTab st_b(false, false);
    // The st_b is tagged by all group ids used in a group, data are the
    // SymTabEnt of st_a that contains the group.

    // String table for st_a, st_b tags.
    strtab_t stringtab;

    char *eqvname = new char[strlen(en_basename) + 8];
    sprintf(eqvname, "%s.equiv", en_basename);
    GCarray<char*> gc_eqvname(eqvname);

    FILE *fp = large_fopen(eqvname, "r");
    if (!fp) {
        Errs()->add_error("stage3: can't open equiv file %s.", eqvname);
        return (false);
    }
    char buf[256];
    char *s = fgets(buf, 256, fp);
    if (!s || !lstring::prefix(EQV_MAGIC, s)) {
        Errs()->add_error(
            "stage3: file %s, format not recognized (bad magic).", eqvname);
        return (false);
    }
    while ((s = fgets(buf, 256, fp)) != 0) {
        if (lstring::prefix(EQV_BEGIN, s))
            break;
    }
    if (!s) {
        Errs()->add_error("stage3: file %s, bad format.", eqvname);
        return (false);
    }

    // Blocks of lines are in grid traversal order, the first token is
    // in the current grid, the second token is from a later grid. 
    // The group numbers are random.

    while ((s = fgets(buf, 256, fp)) != 0) {
        if (*s == '#') {
            if (lstring::prefix(EQV_END, s))
                break;
            continue;
        }

        int x1, y1, g1, x2, y2, g2;
        if (sscanf(s, "%d %d %d %d %d %d",
                &x1, &y1, &g1, &x2, &y2, &g2) != 6) {
            Errs()->add_error("stage3: bad record in file %s.", eqvname);
            return (false);
        }
        sprintf(buf, "%d_%d_%d", x1, y1, g1);
        const char *id1 = stringtab.add(buf);
        sprintf(buf, "%d_%d_%d", x2, y2, g2);
        const char *id2 = stringtab.add(buf);

        SymTabEnt *h1 = (SymTabEnt*)st_b.get(id1);
        if (h1 == (SymTabEnt*)ST_NIL)
            h1 = 0;

        SymTabEnt *h2 = (SymTabEnt*)st_b.get(id2);
        if (h2 == (SymTabEnt*)ST_NIL)
            h2 = 0;

        // All of the connected nets are associated into a group,
        // which is keyed by the lowest traversal-order element. 
        // These are accumulated in the st_a table.  The st_b table
        // contains an entry for every net which appears in a group,
        // and points to its group.  Singletons don't appear in either
        // table.

        if (h1) {
            if (h2) {
                // Both are known.  If they are in different groups,
                // merge the groups, keeping the earliest tag (in
                // traversal order).

                if (h1 != h2) {
                    if (test_order(h1->stTag, h2->stTag) > 0) {
                        SymTabEnt *h = h1;
                        h1 = h2;
                        h2 = h;
                    }
                    SymTab *st1 = (SymTab*)h1->stData;
                    SymTab *st2 = (SymTab*)h2->stData;

                    SymTabGen gen(st2);
                    SymTabEnt *h;
                    while ((h = gen.next()) != 0) {
                        st1->add(h->stTag, 0, true);
                        st_b.replace(h->stTag, h1);
                    }
                    st1->add(h2->stTag, 0, true);
                    st_b.replace(h2->stTag, h1);
                    delete st2;
                    h2->stData = 0;
                    st_a->remove(h2->stTag);
                }
            }
            else {
                // Id1 known, id2 not seen before.
                // Add id2 to the existing group.

                SymTab *st = (SymTab*)h1->stData;
                st->add(id2, 0, true);

                st_b.add(id2, h1, false);
            }
        }
        else {
            if (h2) {
                // Id1 not seen before, id2 known.
                if (test_order(id1, h2->stTag) > 0) {
                    // Add id1 to the existing group.

                    SymTab *st = (SymTab*)h2->stData;
                    st->add(id1, 0, true);

                    st_b.add(id1, h2, false);
                }
                else {
                    // Move group to id1, which becomes the new tag.

                    SymTab *st = (SymTab*)h2->stData;
                    h2->stData = 0;
                    st->add(h2->stTag, 0, false);
                    st_a->remove(h2->stTag);

                    st_a->add(id1, 0, false);
                    h1 = st_a->get_ent(id1);
                    h1->stData = st;

                    st_b.add(id1, h1, false);
                    SymTabGen gen(st);
                    SymTabEnt *h;
                    while ((h = gen.next()) != 0)
                        st_b.replace(h->stTag, h1);
                }
            }
            else {
                // Neither name seen before.  We know that id2 follows
                // id1 in traversal order.

                st_a->add(id1, 0, false);
                SymTabEnt *h = st_a->get_ent(id1);
                SymTab *st = new SymTab(false, false);
                h->stData = st;
                st->add(id2, 0, false);

                st_b.add(id1, h, false);
                st_b.add(id2, h, false);
            }
        }
    }
    fclose(fp);

    if (!(en_flags & EN_KEEP))
        unlink(eqvname);

    // Basename buffer, used for OASIS grid file names.
    char *fname = new char[strlen(en_basename) + 40];
    char *fptr = lstring::stpcpy(fname, en_basename);
    GCarray<char*> gc_bname(fname);

    char *outname = new char[strlen(en_basename) + 5];
    sprintf(outname, "%s.oas", en_basename);
    GCarray<char*> gc_outname(outname);

    comp_setup cs;
    if (en_flags & EN_COMP)
        cs.set();
    oas_out *oas = new oas_out(0);
    if (en_flags & EN_COMP)
        cs.clear();

    GCobject<oas_out*> gc_oas(oas);
    if (!oas->set_destination(outname)) {
        Errs()->add_error("stage3: can't open destination file %s.", outname);
        return (false);
    }
    if (!oas->write_begin(1.0)) {
        Errs()->add_error("stage3: write_begin returned error.");
        return (false);
    }

    // Traverse the grid and write the net cells.  The connected
    // nets become subcells in the tagging net, if not in flat
    // mode.

    // When flattening, we need to open multiple files at once, so we keep
    // the CHDs around.
    cCHD **chd_cache = new cCHD*[en_nx*en_ny];
    memset(chd_cache, 0, en_nx*en_ny*sizeof(cCHD*));

    // Buffer for cell names as in tables and grid files.
    char *hname = buf;

    // Buffer for output cell names.
    char *cname = buf + 128;

    bool flat = (en_flags & EN_FLAT);
    bool ok = true;
    for (int ic = 0; ic < en_ny; ic++) {
        for (int jc = 0; jc < en_nx; jc++) {
            sprintf(fptr, "_%d_%d.oas", jc, ic);
            cCHD *chd = chd_cache[ic*en_nx + jc];
            if (!chd) {
                chd = FIO()->NewCHD(fname, Foas, Physical, 0);
                if (!chd) {
                    Errs()->add_error(
                        "stage3: failed to create CHD for %s.", fname);
                    ok = false;
                    break;
                }
                chd_cache[ic*en_nx + jc] = chd;
            }

            cv_in *in = setup_input(chd, oas);
            if (!in) {
                Errs()->add_error("stage3: input setup failed for %s.",
                    fname);
                ok = false;
                break;
            }

            for (int grp = 0; ; grp++) {
                sprintf(hname, "%d_%d_%d", jc, ic, grp);
                symref_t *p = chd->findSymref(hname, Physical);
                if (!p)
                    // Done, no more nets in this file.
                    break;

                bool primary = st_a->get(hname) != ST_NIL ||
                    st_b.get(hname) == ST_NIL;

                if (!flat || primary) {

                    if (primary) {
                        en_netcnt++;
                        sprintf(cname, "n%d", en_netcnt);
                    }
                    else
                        strcpy(cname, hname);

                    if (!oas->write_begin_struct(cname)) {
                        Errs()->add_error(
                            "stage3: write_begin_struct %s failed.", cname);
                        ok = false;
                        break;
                    }

                    oas->set_no_struct(true);
                    if (!in->chd_read_cell(p, false)) {
                        Errs()->add_error("stage3: read cell %s failed.",
                            hname);
                        ok = false;
                        break;
                    }

                    SymTabEnt *h = st_a->get_ent(hname);
                    if (h) {
                        SymTab *st = (SymTab*)h->stData;
                        stringlist *names = st->names();
                        ok = add_listed_nets(flat, names, oas, chd_cache, in);
                        names->free();
                        if (!ok)
                            break;
                    }
                    oas->set_no_struct(false);

                    if (!oas->write_end_struct(cname)) {
                        Errs()->add_error(
                            "stage3: write_end_struct %s failed.", cname);
                        ok = false;
                        break;
                    }
                }
            }
            in->chd_finalize();
            delete in;

            // In either mode, this and the CHDs for grid cells already
            // processed are no longer needed.
            if (!(en_flags & EN_KEEP))
                unlink(chd->filename());
            delete chd;
            chd_cache[ic*en_nx + jc] = 0;

            if (!ok)
                break;
        }
        if (!ok)
            break;
    }

    int sz = en_nx*en_ny;
    for (int i = 0; i < sz; i++)
        delete chd_cache[i];
    delete [] chd_cache;

    if (!ok)
        return (false);

    // Create top-level cell and add instances of net cells.

    if (!oas->write_begin_struct(en_cellname)) {
        Errs()->add_error("stage3: write_begin_struct %s failed.",
            en_cellname);
        return (false);
    }
    Instance inst;
    for (unsigned int i = 1; i <= en_netcnt; i++) {
        sprintf(cname, "n%d", i);
        inst.name = cname;
        if (!oas->write_sref(&inst)) {
            Errs()->add_error("stage3: write_sref %s failed.", cname);
            return (false);
        }
    }
    if (!oas->write_end_struct(en_cellname)) {
        Errs()->add_error("stage3: write_end_struct %s failed.",
            en_cellname);
        return (false);
    }
    if (!oas->write_endlib(en_cellname)) {
        Errs()->add_error("stage3: write_endlib failed.");
        return (false);
    }

    return (true);
}


bool
cExtNets::add_listed_nets(bool flat, stringlist *names, oas_out *oas,
    cCHD **chd_cache, cv_in *ref_in) const
{
    if (flat) {
        char *fname = new char[strlen(en_basename) + 40];
        char *fptr = lstring::stpcpy(fname, en_basename);
        GCarray<char*> gc_fname(fname);

        // Sort the names, so that names from the same grid file will be
        // grouped.  A lexical sort is good enough.
        names->sort(0);

        for (stringlist *n = names; n; n = n->next) {
            int x, y, g;
            if (sscanf(n->string, "%d_%d_%d", &x, &y, &g) != 3) {
                Errs()->add_error(
                    "add_listed_nets: id string %s parse failed.",
                    n->string);
                return (false);
            }
            cCHD *chd = chd_cache[y*en_nx + x];
            if (ref_in && chd && ref_in->cur_chd() == chd) {

                // This CHD is already open in the caller.  We can't
                // create a second cv_in since the existing cv_in has
                // the header table data.  All the better, since we
                // can bypass this.

                symref_t *p = chd->findSymref(n->string, Physical);
                if (!p) {
                    Errs()->add_error(
                        "add_listed_nets: symref %s unresolved in %s.",
                        n->string, lstring::strip_path(chd->filename()));
                    return (false);
                }
                oas->set_no_struct(true);
                if (!ref_in->chd_read_cell(p, false)) {
                    Errs()->add_error(
                        "add_listed_nets: read cell %s from %s failed.",
                        n->string, lstring::strip_path(chd->filename()));
                    return (false);
                }
                continue;
            }

            if (!chd) {
                sprintf(fptr, "_%d_%d.oas", x, y);
                chd = FIO()->NewCHD(fname, Foas, Physical, 0);
                if (!chd) {
                    Errs()->add_error(
                        "add_listed_nets: failed to create CHD for %s.",
                        fname);
                    return (false);
                }
                chd_cache[y*en_nx + x] = chd;
            }

            cv_in *in = setup_input(chd, oas);
            if (!in) {
                Errs()->add_error(
                    "add_listed_nets: input setup failed for %s.", fname);
                return (false);
            }

            for (;;) {
                symref_t *p = chd->findSymref(n->string, Physical);
                if (!p) {
                    Errs()->add_error(
                        "add_listed_nets: symref %s unresolved in %s.",
                        n->string, fname);
                    in->chd_finalize();
                    delete in;
                    return (false);
                }
                oas->set_no_struct(true);
                if (!in->chd_read_cell(p, false)) {
                    Errs()->add_error(
                        "add_listed_nets: read cell %s from %s failed.",
                        n->string, fname);
                    in->chd_finalize();
                    delete in;
                    return (false);
                }
                if (n->next) {
                    int xx, yy, gg;
                    if (sscanf(n->next->string, "%d_%d_%d",
                            &xx, &yy, &gg) != 3) {
                        Errs()->add_error(
                            "add_listed_nets: id string %s parse failed.",
                            n->next->string);
                        in->chd_finalize();
                        delete in;
                        return (false);
                    }
                    if (xx == x && yy == y) {
                        n = n->next;
                        continue;
                    }
                }
                break;
            }

            in->chd_finalize();
            delete in;
        }
    }
    else {
        Instance inst;
        for (stringlist *n = names; n; n = n->next) {
            inst.name = n->string;
            if (!oas->write_sref(&inst)) {
                Errs()->add_error("add_listed-nets: write_sref %s failed.",
                    n->string);
                return (false);
            }
        }
    }
    return (true);
}
// End of cExtNets functions.


emrec_t *
emrec_t::sort_edge()
{
    emrec_t *eml0 = 0, *emle = 0;
    emrec_t *emlist = this;
    while (emlist) {
        emrec_t *em0 = emlist;
        emrec_t *ee = emlist;
        while (ee->next && ee->next->lname == em0->lname)
            ee = ee->next;
        emlist = ee->next;
        ee->next = 0;

        em0 = em0->sort();  // include merge

        if (!eml0)
            eml0 = emle = em0;
        else {
            while (emle->next)
                emle = emle->next;
            emle->next = em0;
        }
    }
    return (eml0);
}


// Write a record, format is:
//   L|B|R|T start end group_num layername
//
void
emrec_t::print(FILE *fp, int edge)
{
    if (edge && strchr("LBRT", edge)) {
        fprintf(fp, "%c %1.4f %1.4f %d %s\n", edge, MICRONS(start),
            MICRONS(end), group, lname);
    }
}


namespace {
    // Sort comparison, ascending in start.
    inline bool
    emcmp(const emrec_t *a, const emrec_t *b)
    {
        return (a->cmpval() < b->cmpval());
    }
}


emrec_t *
emrec_t::sort()
{
    int cnt = 0;
    for (emrec_t *em = this; em; em = em->next)
        cnt++;
    if (cnt < 2)
        return (this);
    emrec_t **ary = new emrec_t*[cnt];
    cnt = 0;
    for (emrec_t *em = this; em; em = em->next)
        ary[cnt++] = em;
    std::sort(ary, ary + cnt, emcmp);
    for (int i = 1; i < cnt; i++)
        ary[i-1]->next = ary[i];
    ary[cnt-1]->next = 0;

    emrec_t *em0 = ary[0];
    delete [] ary;

    for (emrec_t *em = em0; em; em = em->next) {
        while (em->next) {
            emrec_t *ex = em->next;
            if (ex->end < em->start || em->end < ex->start)
                break;
            if (ex->start < em->start)
                em->start = ex->start;
            if (ex->end > em->end)
                em->end = ex->end;
            emrec_t *et = em->next;
            em->next = et->next;
            delete et;
        }
    }
    return (em0);
}
// End of emrec_t functions.

