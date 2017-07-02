
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.        *
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
 $Id: tech_ldb3d.cc,v 1.17 2017/03/14 01:26:55 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_ylist.h"
#include "cd_lgen.h"
#include "cd_propnum.h"
#include "tech.h"
#include "tech_ldb3d.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "errorlog.h"

//
// A layer sequencing and 3-d database, for cross-section analysis and
// parasitic extraction.
//


//#define DEBUG

unsigned int Ldb3d::db3_zlimit = DB3_DEF_ZLIMIT;
bool Ldb3d::db3_logging = false;

Ldb3d::Ldb3d()
{
    db3_sdesc = 0;
    db3_subseps = 0.0;
    db3_substhick = 0.0;
    db3_zlref = 0;
    db3_stack = 0;
    db3_groups = 0;
    db3_nlayers = 0;
    db3_ngroups = 0;
    db3_is_cross_sect = false;
    db3_logfp = 0;
}


Ldb3d::~Ldb3d()
{
    Zlist::destroy(db3_zlref);
    db3_stack->free();
    delete db3_groups;
}


namespace {
    // Return true if this layer is to be sequenced.
    //
    bool filter3d(const CDl *ld)
    {
        // Can't be invisible of symbolic.
        if (ld->isInvisible() || ld->isSymbolic())
            return (false);

        // Must be an insulator or conductor, as defined for extraction
        // and cross-sections.
        if (!Layer3d::is_conductor(ld) && !Layer3d::is_insulator(ld))
            return (false);

        // Must have finite thickness.
        if (dsp_prm(ld)->thickness() <= 0)
            return (false);

        // Do some sanity checking of the VIA layer.
        if (ld->isVia()) {
            bool ok = true;
            for (sVia *vl = tech_prm(ld)->via_list(); vl; vl = vl->next()) {
                if (!vl || !vl->layer1() || !vl->layer2()) {
                    Log()->WarningLogV(mh::Techfile,
                        "VIA layer %s: missing associated conductor, "
                        "layer ignored.", ld->name());
                    ok = false;
                    break;
                }
                if (!Layer3d::is_conductor(vl->layer1())) {
                    Log()->WarningLogV(mh::Techfile,
                        "VIA layer %s: reference to %s, not a conductor, "
                        "layer ignored.", ld->name(), vl->layer1()->name());
                    ok = false;
                    break;
                }
                if (!Layer3d::is_conductor(vl->layer2())) {
                    Log()->WarningLogV(mh::Techfile,
                        "VIA layer %s: reference to %s, not a conductor, "
                        "layer ignored.", ld->name(), vl->layer2()->name());
                    ok = false;
                    break;
                }
            }
            if (!ok)
                return (false);
        }
        return (true);
    }
}


// Find and order the process layers.  The initial order is set from
// technology information only, and each sequenceable layer will be
// included.
//
// The Errs list is initialized here, and should be checked for
// messages on return.  Messages with a true return are considered
// warnings.
//
bool
Ldb3d::order_layers()
{
    // New CDL flags:
    //
    // DIELECTRIC
    // Specifies that the layer is a dielectric to be used by this
    // interface, provided Thickness is set positive and EpsRel is
    // given.  Can not also have VIA or a conductor keyword set.
    //
    // PLANAR[IZE] yes|no
    // Controls whether or not the layer is treated as planarizing by
    // this interface.  If a layer is planarizing, when the layer
    // geometry is being extracted, a first step is to find the
    // highest elevation in the layout, and to fill the empty space in
    // the layout up to this level with layer material.  The top
    // surface of the layout is then planar.  Then, the regular layer
    // features are added.  If not given, CDL_CONDUCTOR and CDL_VIA
    // layers are "yes", all others are "no", unless the NoPlanarize 
    // variable is set, in which case no layers are planarizing by
    // default.

    db3_stack->free();
    db3_stack = 0;

    Layer3d *lend = 0;
    CDll *ll0 = Tech()->sequenceLayers(&filter3d);
    for (CDll *ll = ll0; ll; ll = ll->next) {
        if (!lend)
            lend = db3_stack = new Layer3d(ll->ldesc, 0);
        else {
            lend->set_next(new Layer3d(ll->ldesc, 0));
            lend->next()->set_prev(lend);
            lend = lend->next();
        }
    }
    CDll::destroy(ll0);

    return (db3_stack != 0);
}


// Check if the dielectric constants are sane.  If not, return false
// with a message in the Errs system.
//
bool
Ldb3d::check_dielectrics()
{
    bool ok = true;
    for (Layer3d *l = db3_stack; l; l = l->next()) {
        if (l->is_insulator()) {
            if (tech_prm(l->layer_desc())->epsrel() < 1.0) {
                Errs()->add_error(
                    "Error, layer %s: dielectric constant less than 1.",
                    l->layer_desc()->name());
                ok = false;
            }
        }
    }
    return (ok);
}


// Dump the layer sequence.  Print thickness in microns and unscaled
// epsrel.
//
void
Ldb3d::layer_dump(FILE *fp) const
{
    bool has_planar = false;
    for (Layer3d *l = db3_stack; l; l = l->next()) {
        if (l->plane() > 0) {
            has_planar = true;
            break;
        }
    }

    char tbuf[32];
    if (db3_substhick > 0.0)
        sprintf(tbuf, "%.3f", db3_substhick);
    if (has_planar) {
        fprintf(fp, "* %-9s %-6s %-12s %-12s %s\n", "Layers", "",
            "Plane", "Thickness", "EpsRel");
        fprintf(fp, "* %-9s %-6s %-12s %-12s %.3f\n", "Substrate", "", "",
            db3_substhick > 0.0 ? tbuf : "infinite", db3_subseps);
    }
    else {
        fprintf(fp, "* %-9s %-6s %-12s %s\n", "Layers", "",
            "Thickness", "EpsRel");
        fprintf(fp, "* %-9s %-6s %-12s %.3f\n", "Substrate", "",
            db3_substhick > 0.0 ? tbuf : "infinite", db3_subseps);
    }
    for (Layer3d *l = db3_stack; l; l = l->next()) {
        double th = dsp_prm(l->layer_desc())->thickness();
        if (l->is_conductor()) {
            fprintf(fp, "* %-9s %-6s ", "Conductor", l->layer_desc()->name());
            if (has_planar)
                fprintf(fp, "%-12.3f ", MICRONS(l->plane()));
            fprintf(fp, "%.3f\n", th);
        }
        else if (l->is_insulator()) {
            fprintf(fp, "* %-9s %-6s ", "Insulator", l->layer_desc()->name());
            if (has_planar)
                fprintf(fp, "%-12.3f ", MICRONS(l->plane()));
            fprintf(fp, "%-12.3f ", th);
            fprintf(fp, "%.3f\n", l->epsrel());
        }
    }
}


namespace {
    int dist(const Point *p, int x, int y)
    {
        if (x == p->x)
            return (abs(y - p->y));
        if (y == p->y)
            return (abs(x - p->x));
        double dx = x - p->x;
        double dy = y - p->y;
        return (mmRnd(sqrt(dx*dx + dy*dy)));
    }
}


// This generates a cross section representation along the line
// between p1 and p2.  The return is an array of box list heads,
// corresponding to the local layer table.
//
Blist **
Ldb3d::line_scan(const Point *p1, const Point *p2)
{
    Blist **lists = new Blist*[db3_nlayers];
    memset(lists, 0, db3_nlayers*sizeof(Blist*));

    BBox pBB(p1->x, p1->y, p2->x, p2->y);
    pBB.fix();
    int lnum = 0;
    for (Layer3d *l = db3_stack; l; l = l->next()) {
        for (glYlist3d *y = l->yl3d(); y; y = y->next) {
            if (y->y_yl > pBB.top)
                continue;
            if (y->y_yu < pBB.bottom)
                break;
            for (glZlist3d *z = y->y_zlist; z; z = z->next) {
                if (z->Z.minleft() > pBB.right)
                    break;
                if (z->Z.maxright() < pBB.left)
                    continue;

                Point pret[2];
                if (z->Z.line_clip(p1->x, p1->y, p2->x, p2->y, pret)) {
                    int d1 = dist(p1, pret[0].x, pret[0].y);
                    int d2 = dist(p1, pret[1].x, pret[1].y);
                    lists[lnum] =
                        new Blist(d1, z->Z.zbot, d2, z->Z.ztop, lists[lnum]);
                }

            }
        }
        lists[lnum] = Blist::merge(lists[lnum]);
        lnum++;
    }
    return (lists);
}


namespace {
    // Return a description of the area for geometry extraction,
    // geometry will be clipped to this area.  If filt_lname matches a
    // physical layer name, and objects on the layer intersect the
    // AOI, all geometry will be clipped to the intersection region.
    //
    Zlist *get_zref(const CDs *sdesc, const BBox *AOI, const char *filt_lname)
    {
        if (!sdesc)
            return (0);
        if (!AOI)
            AOI = sdesc->BB();
        Zlist *zref = new Zlist(AOI);
        if (filt_lname) {
            CDl *fld = CDldb()->findLayer(filt_lname, Physical);
            if (fld) {
                XIrt ret;
                Zlist *zl = sdesc->getZlist(CDMAXCALLDEPTH, fld, zref, &ret);
                if (ret != XIok) {
                    Zlist::destroy(zref);
                    return (0);
                }
                if (zl) {
                    Zlist::destroy(zref);
                    zref = zl;
                }
            }
        }
        return (zref);
    }
}


bool
Ldb3d::init_stack(CDs *sdesc, const BBox *AOI, bool is_cs,
    const char *mask_lname, double subs_eps, double subs_thickness)
{
    // Grab and order the layers to be considered, in the db3_stack
    // list.

    db3_sdesc = sdesc;
    db3_subseps = subs_eps;
    db3_substhick = subs_thickness;
    db3_is_cross_sect = is_cs;

    if (!order_layers())
        return (false);

    if (db3_logfp) {
        fprintf(db3_logfp, "Layer sequencing succeeded, sequence is:\n");
        for (Layer3d *l = db3_stack; l; l = l->next())
            fprintf(db3_logfp, " %s", l->layer_desc()->name());
        fprintf(db3_logfp, "\n");
    }

    // If no sdesc is passed, we still run the layer ordering, which
    // might be useful stand-alone.
    if (!sdesc)
        return (true);

    // Find the area of interest, to which geometry will be clipped. 
    // This will be the intersection of the cell BB, the AOI, and any
    // regions of a layer named in mask_lname.

    Zlist *zref = get_zref(sdesc, AOI, mask_lname);
    if (!zref) {
        Errs()->add_error(
            "Failed to determine reference area for geometry extraction.");
        return (false);
    }
    Zlist::BB(zref, db3_aoi);
    Zlist::destroy(db3_zlref);
    db3_zlref = zref;

    // Next, obtain geometry, and remove layers that are nonexistant
    // in the sdesc layout.  We invert dark-field layers, so that in
    // the sequence, the absence of those layers indicates "everywhere
    // present", thus we would keep the layers.  We keep layers that
    // are planarizing in any case.

    unsigned int zcnt = 0;
    Layer3d *lp = 0, *lnxt;
    for (Layer3d *l = db3_stack; l; l = lnxt) {
        lnxt = l->next();
        l->extract_geom(sdesc, db3_zlref);
        unsigned int zc = Ylist::count_zoids(l->uncut());
        if (db3_logfp && zc) {
            fprintf(db3_logfp, "Trapezoid count for %s is %u.\n",
                l->layer_desc()->name(), zc);
        }
        zcnt += zc;
        if (zcnt > zoid_limit()) {
            Errs()->add_error(
                "Exceeded 3-d database raw trapezoid limit of %u, aborting.",
                zoid_limit());
            return (false);
        }

        // If the layer is not planarizing and no material is present,
        // remove the layer from the sequence.  If planarizing, we
        // have to keep it, it will still be used for planarization.

        if (!l->layer_desc()->isPlanarizing() && !l->uncut()) {
            if (lp)
                lp->set_next(lnxt);
            else
                db3_stack = lnxt;
            if (lnxt)
                lnxt->set_prev(lp);
            if (db3_logfp) {
                fprintf(db3_logfp,
                    "No material for %s, removing from sequence.\n",
                    l->layer_desc()->name());
            }
            delete l;
            continue;
        }
        lp = l;
    }
    if (db3_logfp) {
        fprintf(db3_logfp, "Total database trapezoids %u.\n", zcnt);
    }

    // Do the cutting.  Since there is no assumed planarization, every
    // edge propagates upward.  We want to cut into regions of
    // homogeneity looking down, i.e., every point in the z-bottom
    // panel of every zoid will lie above the same layer stack, and
    // thus have a constant elevation.

    db3_stack->cut(0);
    for (Layer3d *l1 = db3_stack; l1; l1 = l1->next()) {
        for (Layer3d *l2 = l1->next(); l2; l2 = l2->next())
            l2->cut(l1);
    }

    // Reformat the 3d trapezoid list.  We keep the original 2d list
    // too.  We assign the layer index numbers here.  We also remove
    // any layers with no geometry, post-planarization.

    int lcnt = 0;
    lp = 0;
    for (Layer3d *l = db3_stack; l; l = lnxt) {
        lnxt = l->next();
        l->set_index(lcnt);
        l->mk3d(db3_is_cross_sect);
        if (!l->yl3d()) {
            if (lp)
                lp->set_next(lnxt);
            else
                db3_stack = lnxt;
            if (lnxt)
                lnxt->set_prev(lp);
            if (db3_logfp) {
                fprintf(db3_logfp,
                    "No material for %s after planarization, removing from "
                    "sequence.\n",
                    l->layer_desc()->name());
            }
            delete l;
            db3_nlayers--;
            continue;
        }
        lp = l;
        lcnt++;
    }
    db3_nlayers = lcnt;

    // The next step is to identify conductor groups
    // three-dimensionally.  We treat all conductors the same.  First
    // go through the layer stack and borrow the lists from conductors
    // into a common list.

    glZlist3d *c0 = 0, *ce = 0;
    for (Layer3d *l = db3_stack; l; l = l->next()) {
        if (l->is_conductor()) {
            glZlist3d *zl = glYlist3d::to_zl3d(l->yl3d());
            l->set_yl3d(0);
            if (!c0)
                c0 = ce = zl;
            else {
                while (ce->next)
                    ce = ce->next;
                ce->next = zl;
            }
        }
    }

    delete db3_groups;
    db3_ngroups = 0;
    glYlist3d *y0 = new glYlist3d(c0);
    glZgroup3d *g = glYlist3d::group(y0);  // consumes y0
    db3_groups = new glZgroupRef3d(g);
    db3_ngroups = g->num;

    // Put the group zoids back into the layer list.
    glZlist3d **ary = new glZlist3d*[db3_nlayers];
    memset(ary, 0, db3_nlayers*sizeof(glZlist3d*));
    for (int i = 0; i < g->num; i++) {
        glZlist3d *zl = g->list[i];
        g->list[i] = 0;
        while (zl) {
            glZlist3d *zx = zl;
            zl = zl->next;
            zx->next = ary[zx->Z.layer_index];
            ary[zx->Z.layer_index] = zx;
        }
    }
    for (Layer3d *l = db3_stack; l; l = l->next()) {
        if (ary[l->index()]) {
            if (l->yl3d())
                Errs()->add_error("Warning: Setup failure,\n"
                    "after 3D processing internal inconsistency detected.");
            l->set_yl3d(new glYlist3d(ary[l->index()]));
            ary[l->index()] = 0;
        }
    }
    int leftovr = 0;
    for (int i = 0; i < db3_groups->num; i++) {
        if (ary[i])
            leftovr++;
    }
    if (leftovr) {
        Errs()->add_error(
            "Warning: Setup failure,\n"
            "3D processing identified %d additional %s.  Are\n"
            "dielectric edges causing conductor discontinuity?", leftovr,
            leftovr == 1 ? "group" : "groups");
    }
    delete [] ary;
    delete g;
    if (db3_logfp)
        fprintf(db3_logfp, "Found %d conductor groups.\n", db3_ngroups);

#ifdef DEBUG
    for (Layer3d *l = db3_stack; l; l = l->next())
        l->add_db();
#endif
    return (true);
}


// Insert the passed VIA layer to the expected position, per the
// present reordering mode.  We know that lx is not already in the
// list.
//
bool
Ldb3d::insert_layer(Layer3d *lx)
{
    if (!lx || !lx->layer_desc()->isVia())
        return (true);
    if (Tech()->ReorderMode() == tReorderNone) {
        // Insert in layer table order.
        int ix = lx->layer_desc()->physIndex();
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (l->layer_desc()->physIndex() < ix && (!l->next() ||
                    l->next()->layer_desc()->physIndex() > ix)) {
                lx->set_next(l->next());
                if (lx->next())
                    lx->next()->set_prev(lx);
                l->set_next(lx);
                lx->set_prev(l);
                return (true);
            }
        }
        if (!db3_stack || ix < db3_stack->layer_desc()->physIndex()) {
            // Must be true, but makes no sense for Via layer to be
            // on the bottom.
            lx->set_next(db3_stack);
            if (lx->next())
                lx->next()->set_prev(lx);
            db3_stack = lx;
            lx->set_prev(0);
        }
        return (true);
    }
    if (Tech()->ReorderMode() == tReorderVabove) {
        CDl *lbot = lx->bottom_layer();
        if (!lbot) {
            // Makes no sense for a Via layer to be on the bottom,
            // but that is the interpretation.
            lx->set_next(db3_stack);
            if (lx->next())
                lx->next()->set_prev(lx);
            db3_stack = lx;
            lx->set_prev(0);
            return (true);
        }
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (l->layer_desc() == lbot) {
                lx->set_next(l->next());
                if (lx->next())
                    lx->next()->set_prev(lx);
                l->set_next(lx);
                lx->set_prev(l);
                return (true);
            }
        }
        return (false);
    }
    if (Tech()->ReorderMode() == tReorderVbelow) {
        CDl *ltop = lx->top_layer();
        if (!ltop) {
            // Makes no sense for a Via layer to be on the top, but
            // that is the interpretation.
            if (!db3_stack) {
                db3_stack = lx;
                lx->set_next(0);
                lx->set_prev(0);
                return (true);
            }
            Layer3d *l = db3_stack;
            while (l->next())
                l = l->next();
            l->set_next(lx);
            lx->set_next(0);
            lx->set_prev(l);
            return (true);
        }
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (l->layer_desc() == ltop) {
                lx->set_next(l);
                if (l->prev()) {
                    lx->set_prev(l->prev());
                    l->prev()->set_next(lx);
                }
                else {
                    db3_stack = lx;
                    lx->set_prev(0);
                }
                l->set_prev(lx);
                return (true);
            }
        }
    }
    return (false);
}


// The argument is one of the elements of the layer, return a pointer
// to the object below Z (closer to the substrate).
//
glZlist3d *
Ldb3d::find_object_under(const glZoid3d *Z) const
{
    if (!Z)
        return (0);
    Layer3d *l = layer(Z->layer_index);
    if (!l)
        return (0);
    int x = (Z->xll + Z->xul + Z->xlr + Z->xur)/4;
    int y = (Z->yl + Z->yu)/2;
    for (l = l->prev(); l; l = l->prev()) {
        glZlist3d *zr = l->find_max_ztop_at(x, y);
        if (zr)
            return (zr);
    }
    return (0);
}


Layer3d::~Layer3d()
{
    Ylist::destroy(l3_cut);
    Ylist::destroy(l3_uncut);
    glYlist3d::destroy(l3_yl3d);
}


// Return the relative dielectric constant, applies to dielectrics
// only.
//
double
Layer3d::epsrel() const
{
    return (l3_ldesc ? tech_prm(l3_ldesc)->epsrel() : 0.0);
}


// Extract the geometry for the layer from sdesc, clipped to the zref. 
// We obtain true polarity, thus shapes correspond to actual material
// on the substrate.  This fills in the l3_uncut field and returns
// true on success.  If there is an error, false is returned.  If
// there is no material, the l3_uncut will be null, which is not an
// error.
//
bool
Layer3d::extract_geom(const CDs *sdesc, const Zlist *zref)
{
    if (!sdesc)
        return (false);
    if (!l3_ldesc)
        return (false);

    // If zref is null, use the cell BB.
    bool free_zref = false;
    if (!zref) {
        zref = new Zlist(sdesc->BB(), 0);
        free_zref = true;
    }

    XIrt ret;
    Zlist *zl = sdesc->getZlist(CDMAXCALLDEPTH, l3_ldesc, zref, &ret);
    if (ret != XIok) {
        if (free_zref)
            Zlist::destroy(zref);
        return (false);
    }
    if (l3_ldesc->isVia() || l3_ldesc->isDarkField()) {
        Zlist *zr = Zlist::copy(zref);
        ret = Zlist::zl_andnot(&zr, zl);
        if (ret != XIok) {
            if (free_zref)
                Zlist::destroy(zref);
            return (false);
        }
        zl = zr;
    }
    if (free_zref)
        Zlist::destroy(zref);

    Ylist::destroy(l3_cut);
    l3_cut = 0;
    Ylist::destroy(l3_uncut);

    zl = Zlist::filter_slivers(zl, 1);
    l3_uncut = zl ? new Ylist(zl) : 0;
    glYlist3d::destroy(l3_yl3d);
    l3_yl3d = 0;
    return (true);
}


// Return true if x,y is enclosed in one of the original trapezoids.
//
bool
Layer3d::intersect(int x, int y) const
{
    Point_c p(x, y);
    return (Ylist::find_container(l3_uncut, &p));
}


// We want all of the boundaries in btm to be reflected in this.  Clip
// this by the btm cut, both clip-to and clip-around, then combine the
// two results into a new cut list.  Note that the cut list is created
// if it doesn't already exist.
//
// We do this in sequence for all layers below.  Then, for each zoid
// in the cut list, we know that all points have the same altitude.
//
bool
Layer3d::cut(const Layer3d *btm)
{
    if (btm == this)
        return (true);
    if (!l3_uncut)
        return (true);
    if (btm == 0) {
        // This signals that we just add the cut field as a copy of
        // uncut, which is appropriate for the lowest layer.

        Ylist::destroy(l3_cut);
        l3_cut = Ylist::copy(l3_uncut);
        return (true);
    }
    
    Ylist *b = l3_cut;
    if (!b)
        b = l3_uncut;
    Ylist *bb = btm->l3_cut;
    if (!bb)
        bb = btm->l3_uncut;
    if (!b || !bb)
        return (true);

    Zlist *z1, *z2;
    try {
        z1 = b->clip_to(bb);
        z2 = b->clip_out(bb);
    }
    catch (XIrt) {
        return (false);
    }

    if (!z1)
        z1 = z2;
    else if (z2) {
        Zlist *zn = z1;
        while (zn->next)
            zn = zn->next;
        zn->next = z2;
    }
    z1 = Zlist::filter_slivers(z1, 1);
    Ylist::destroy(l3_cut);
    l3_cut = new Ylist(z1);
    return (true);
}


// Create a 3d trapezoid list from the cut list, the cut list is
// freed.
//
void
Layer3d::mk3d(bool is_cross_sect)
{
    Zlist *zl0 = Ylist::to_zlist(l3_cut);
    l3_cut = 0;
    glYlist3d::destroy(l3_yl3d);
    l3_yl3d = 0;
    glZlist3d *z3 = 0, *z3e = 0;
    while (zl0) {
        Zlist *zx = zl0;
        int x = (zx->Z.xll + zx->Z.xul + zx->Z.xlr + zx->Z.xur)/4;
        int y = (zx->Z.yl + zx->Z.yu)/2;

        int bot = 0;
        for (Layer3d *l = prev(); l; l = l->prev()) {
            glZlist3d *zt = l->find_max_ztop_at(x, y);
            if (zt) {
                bot = zt->Z.ztop;
                break;
            }
        }
        int t = INTERNAL_UNITS(dsp_prm(l3_ldesc)->thickness());
        if (is_cross_sect && dsp_prm(l3_ldesc)->xsect_thickness() > 0)
            t = dsp_prm(l3_ldesc)->xsect_thickness();
        int top = bot + t;

        if (!z3)
            z3 = z3e = new glZlist3d(zx->Z, bot, top, l3_index, -1);
        else {
            z3e->next = new glZlist3d(zx->Z, bot, top, l3_index, -1);
            z3e = z3e->next;
        }

        zl0 = zl0->next;
        delete zx;
    }
    if (z3)
        l3_yl3d = new glYlist3d(z3);
    if (l3_ldesc->isPlanarizingSet()) {
        if (l3_ldesc->isPlanarizing())
            planarize();
    }
    else if (!Tech()->IsNoPlanarize() &&
            (l3_ldesc->isVia() || l3_ldesc->isConductor()))
        planarize();

}


// Return a pointer to the element under x,y with the largest ztop.
//
glZlist3d *
Layer3d::find_max_ztop_at(int x, int y) const
{
    Point_c p(x, y);
    glZlist3d *zmx = 0;
    for (glYlist3d *yl = l3_yl3d; yl; yl = yl->next) {
        if (yl->y_yu <= y)
            break;
        if (yl->y_yl >= y)
            continue;
        for (glZlist3d *z = yl->y_zlist; z; z = z->next) {
            if (z->Z.yl >= y)
                continue;
            if (z->Z.maxright() <= x)
                continue;
            if (z->Z.minleft() >= x)
                break;
            if (((Zoid)z->Z).intersect(&p, false)) {
                if (!zmx || (z->Z.ztop > zmx->Z.ztop))
                    zmx = z;
            }
        }
    }
    return (zmx);
}


// Add 3d-zoids so as to make the surface planar, below the normal
// 3d-zoids from the list.
//
void
Layer3d::planarize()
{
    // Find the highest elevation zbot.
    int zmax = 0;
    for (glYlist3d *y = yl3d(); y; y = y->next) {
        for (glZlist3d *z = y->y_zlist; z; z = z->next) {
            if (z->Z.zbot > zmax)
                zmax = z->Z.zbot;
        }
    }

    // Set the plane field, which indicates that the layer is
    // planarizing.
    l3_plane = zmax;

    // Reset the ztop value, retaining the zbot value, and we're done.
    for (glYlist3d *y = yl3d(); y; y = y->next) {
        for (glZlist3d *z = y->y_zlist; z; z = z->next)
            z->Z.ztop = zmax + (z->Z.ztop - z->Z.zbot);
    }
}


// If this is a VIA, return the bottom layer implied by the sVia list. 
// On error, return 0.
//
CDl *
Layer3d::bottom_layer()
{
    if (layer_desc()->isVia()) {
        sVia *vl0 = tech_prm(layer_desc())->via_list();
        if (!vl0)
            return (0);
        CDl *lbot = vl0->bottom_layer();
        if (!lbot)
            return (0);
        for (sVia *vl = vl0->next(); vl; vl = vl->next()) {
            CDl *tb = vl->bottom_layer();
            if (!tb)
                return (0);
            if (tb->physIndex() > lbot->physIndex())
                lbot = tb;
        }
        return (lbot);
    }
    return (0);
}


// If this is a VIA, return the top layer implied by the sVia list. 
// On error, return 0.
//
CDl *
Layer3d::top_layer()
{
    if (layer_desc()->isVia()) {
        sVia *vl0 = tech_prm(layer_desc())->via_list();
        if (!vl0)
            return (0);
        CDl *ltop = vl0->top_layer();
        if (!ltop)
            return (0);
        for (sVia *vl = vl0->next(); vl; vl = vl->next()) {
            CDl *tt = vl->top_layer();
            if (!tt)
                return (0);
            if (tt->physIndex() < ltop->physIndex())
                ltop = tt;
        }
        return (ltop);
    }
    return (0);
}


// Static function.
// Return true if the layer is a conductor, which includes resistor
// layers.
//
bool
Layer3d::is_conductor(const CDl *ld)
{
    if (!ld)
        return (false);
    if (ld->flags() &
            (CDL_CONDUCTOR | CDL_ROUTING | CDL_GROUNDPLANE | CDL_IN_CONTACT))
        return (true);
    if (ld->flags() & (CDL_VIA | CDL_DIELECTRIC))
        return (false);
    if (tech_prm(ld)->rho() > 0.0 || tech_prm(ld)->ohms_per_sq() > 0.0 ||
            tech_prm(ld)->lambda() > 0.0)
        return (true);
    return (false);
}


// Static function.
// Return true if the layer is an insulator.
//
bool
Layer3d::is_insulator(const CDl *ld)
{
    if (!ld)
        return (false);
    return (ld->flags() & (CDL_VIA | CDL_DIELECTRIC));
}


// Put the zoids in the main database, for debugging.
//
bool
Layer3d::add_db()
{
    char buf[64];
    sprintf(buf, "%s:ldb3d", layer_desc()->name());
    CDl *ld = CDldb()->newLayer(buf, Physical);
    bool undoable = true;
    bool use_merge = false;
    CDs *sdesc = CurCell(Physical);
    if (!sdesc || !ld)
        return (false);

    Errs()->init_error();
    for (glYlist3d *y = yl3d(); y; y = y->next) {
        for (glZlist3d *z = y->y_zlist; z; z = z->next) {
            if (z->Z.xll == z->Z.xul && z->Z.xlr == z->Z.xur) {
                if (z->Z.xll == z->Z.xlr || z->Z.yl >= z->Z.yu) {
                    // bad zoid, ignore
                    printf("Bad zoid (Manhattan)\n");
                    continue;
                }
                BBox tBB;
                tBB.left = z->Z.xll;
                tBB.bottom = z->Z.yl;
                tBB.right = z->Z.xur;
                tBB.top = z->Z.yu;
                if (tBB.left == tBB.right || tBB.top == tBB.bottom)
                    continue;
                CDo *newo;
                if (sdesc->makeBox(ld, &tBB, &newo) != CDok) {
                    Errs()->add_error("makeBox failed");
                    GEO()->ifInfoMessage(IFMSG_LOG_ERR, Errs()->get_error());
                    continue;
                }
                if (undoable) {
                    GEO()->ifRecordObjectChange(sdesc, 0, newo);
                    if (use_merge && !sdesc->mergeBoxOrPoly(newo, true)) {
                        Errs()->add_error("mergeBoxOrPoly failed");
                        GEO()->ifInfoMessage(IFMSG_LOG_ERR,
                            Errs()->get_error());
                        continue;
                    }
                }
            }
            else {
                if (z->Z.xll > z->Z.xlr || z->Z.xul > z->Z.xur ||
                        (z->Z.xll == z->Z.xlr && z->Z.xul == z->Z.xur) ||
                        z->Z.yl >= z->Z.yu) {
                    // bad zoid, ignore
                    printf("Bad zoid\n");
                    continue;
                }

                Poly poly;
                if (z->Z.mkpoly(&poly.points, &poly.numpts, false)) {
                    CDpo *newo;
                    if (sdesc->makePolygon(ld, &poly, &newo) != CDok) {
                        Errs()->add_error("makePolygon failed");
                        GEO()->ifInfoMessage(IFMSG_LOG_ERR,
                            Errs()->get_error());
                        continue;
                    }
                    if (undoable) {
                        GEO()->ifRecordObjectChange(sdesc, 0, newo);
                        if (use_merge && !sdesc->mergeBoxOrPoly(newo, true)) {
                            Errs()->add_error("mergeBoxOrPoly failed");
                            GEO()->ifInfoMessage(IFMSG_LOG_ERR,
                                Errs()->get_error());
                            continue;
                        }
                    }
                }
            }
        }
    }
    return (true);
}
// End of Layer3d functions.

