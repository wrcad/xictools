
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

#include "main.h"
#include "sced.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_errlog.h"
#include "edit.h"
#include "undolist.h"
#include "tech_layer.h"
#include "sced_nodemap.h"
#include "cd_terminal.h"
#include "cd_celldb.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_netname.h"
#include "cd_lgen.h"
#include "geo_zlist.h"
#include "dsp_inlines.h"
#include "errorlog.h"


// Update the net name labels of all cells in the current hierarchy,
// from the sGroup::netlist().
//
void
cExt::updateNetLabels()
{
    CDs *sd = CurCell(Physical);
    if (!sd)
        return;
    if (!associate(sd))
        return;

    Ulist()->ListCheck("netlb", sd, false);
    cGroupDesc *gd = sd->groups();
    if (gd) {
        if (!gd->update_net_labels())
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
    }

    bool ok = true;
    CDgenHierDn_s gen(sd);
    CDs *tsd;
    bool err;
    while ((tsd = gen.next(&err)) != 0) {
        if (tsd == sd)
            continue;
        Ulist()->ListChangeCell(tsd);
        gd = tsd->groups();
        if (!gd)
            continue;
        if (!gd->update_net_labels())
            ok = false;
    }
    if (err)
        ok = false;
    if (!ok && Errs()->has_error())
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
    Ulist()->ListChangeCell(sd);
    Ulist()->CommitChanges(true);
}


// Return a possibly new layer, the same layernum as ld, but with the
// PIN (or equivalent) purpose.  The passed layer must be a conductor. 
// The PIN layer is used for labels associated with the net.
//
CDl *
cExt::getPinLayer(const CDl *ld, bool create)
{
    if (!ld || !ld->isConductor())
        return (0);
    TechLayerParams *lp = tech_prm(ld);
    CDl *pld = lp->pin_layer();
    if (pld)
        return (pld);

    // If there is a "PinLayer" name set, and the layer exists, it
    // will override the PinPurpose mechanism.
    const char *pinlyr = CDvdb()->getVariable(VA_PinLayer);
    if (pinlyr) {
        pld = CDldb()->findLayer(pinlyr, Physical);
        EX()->setPinLayer(pld);
        if (pld) {
            lp->set_pin_layer(pld);
            return (pld);
        }
    }

    const char *pinprp = CDvdb()->getVariable(VA_PinPurpose);
    if (!pinprp)
        pinprp = EXT_DEF_PIN_PURPOSE;
    else if (!*pinprp)
        pinprp = CDL_PRP_DRAWING;

    bool unkn;
    unsigned int pnum = CDldb()->getOApurposeNum(pinprp, &unkn);
    if (unkn) {
        if (!create)
            return (0);
        pnum = CDldb()->newPurposeNum();
        CDldb()->saveOApurpose(pinprp, pnum, 0);
    }
    pld = CDldb()->findLayer(ld->oaLayerNum(), pnum, Physical);
    if (pld) {
        lp->set_pin_layer(pld);
        return (pld);
    }
    if (!create)
        return (0);
    return (CDldb()->newLayer(ld->oaLayerNum(), pnum, Physical));
}


// Return a name for the group, either the netname, or the name of a
// formal terminal.  In the latter case, has_cterm is set.  If there
// is both a netname and a terminal name and they differ, set the
// has_conflict return.  If no name, return 0.
//
const char *
cGroupDesc::group_name(int grp, bool *has_cterm, bool *has_conflict) const
{
    *has_cterm = false;
    *has_conflict = false;
    sGroup *g = group_for(grp);
    if (g) {
        for (CDpin *p = g->termlist(); p; p = p->next()) {
            *has_cterm = true;
            if (p->term()->name() != g->netname()) {
                if (!g->netname() ||
                        g->netname_origin() < sGroup::NameFromTerm)
                    g->set_netname(p->term()->name(),
                        sGroup::NameFromTerm);
                else
                    *has_conflict = true;
            }
        }
        if (gd_groups[grp].netname())
            return (Tstring(gd_groups[grp].netname()));
    }
    return (0);
}


// Return a name for the group, either an applied text name, or the
// group number.  If ignore names, always return the number.  If
// map_lstr, add a mapping string record.
//
const char *
cGroupDesc::group_name(int grp, sLstr *map_lstr) const
{
    if (grp < 0)
        return (Tstring(CDnetex::name_tab_add("-1")));
    sGroup *g = group_for(grp);
    if (!g)
        return (Tstring(CDnetex::name_tab_add("-1")));

    const char *nm = Tstring(g->netname());
    if (nm && (!EX()->isIgnoreGroupNames() || SCD()->isGlobalNetName(nm)))
        return (nm);

    if (!EX()->isIgnoreGroupNames() && map_lstr) {
        CDnetName cnm = 0;
        for (CDpin *p = gd_groups[grp].termlist(); p; p = p->next()) {
            if (!p->term()->instance()) {
                // found a cell terminal
                cnm = p->term()->name();
                break;
            }
        }
        if (cnm)
            gd_groups[grp].set_netname(cnm, sGroup::NameFromTerm);

        map_lstr->add("* ");
        map_lstr->add_i(grp);
        map_lstr->add_c(' ');
        map_lstr->add(cnm ? Tstring(cnm) : "???");
        map_lstr->add_c('\n');
    }

    char buf[64];
    mmItoA(buf, grp);
    return (Tstring(CDnetex::name_tab_add(buf)));
}


// Initialize the sGroup::netnames from the electrical nets.  Call
// this after association succeeds.
//
void
cGroupDesc::init_net_names()
{
    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);

    // Name the physical nets from the named electrical nets.  We
    // apply the names to unnamed physical nets only.

    cNodeMap *m = esdesc ? esdesc->nodes() : 0;
    for (int i = 1; i < gd_asize; i++) {
        sGroup &g = gd_groups[i];
        int n = g.node();
        if (n > 0) {
            CDnetName nm = m ? m->mapStab(n) : 0;
            if (nm && !g.netname())
                g.set_netname(nm, sGroup::NameFromNode);
        }
    }
}


// Update the net name labels in the cell from the sGroup::netnames.
//
bool
cGroupDesc::update_net_labels()
{
    for (int i = 1; i < gd_asize; i++) {
        if (!update_net_label(i))
            return (false);
    }
    return (true);
}


// Update the net name label for the given group number.  The new name
// is from the sGroup::netname().
//
bool
cGroupDesc::update_net_label(int grp)
{
    if (grp < 1 || grp >= gd_asize)
        return (0);
    CDol *list = find_net_labels(grp, 0);
    const char *newstr = Tstring(gd_groups[grp].netname());
    if (!list && newstr)
        return (create_net_label(grp, newstr, false, 0));

    for (CDol *ol = list; ol; ol = ol->next) {
        if (!newstr)
            Ulist()->RecordObjectChange(gd_celldesc, ol->odesc, 0);
        else {
            CDla *la = (CDla*)ol->odesc;
            char *s = hyList::string(la->label(), HYcvPlain, true);
            if (!s || strcmp(s, newstr)) {
                Label label;
                label.label = new hyList(0, newstr, HYcvPlain); 
                label.x = la->xpos();
                label.y = la->ypos();
                label.xform = la->xform();
                if (!gd_celldesc->newLabel(la, &label, la->ldesc(), 0,
                        false)) {
                    delete [] s;
                    Errs()->add_error(
                        "update_net_label: label creation failed.");
                    return (false);
                }
            }
            delete [] s;
        }
    }
    CDol::destroy(list);
    return (true);
}


// Create an associated net name label for the group whose number is
// passed.  If check is true, see if a label containing the same text
// already exists and if so don't create the new one.
//
bool
cGroupDesc::create_net_label(int grp, const char *string, bool check,
    CDla **newp)
{
    if (newp)
        *newp = 0;
    if (grp < 1 || grp >= gd_asize) {
        Errs()->add_error("create_net_label: group index out of range.");
        return (false);
    }
    if (!string || !*string) {
        Errs()->add_error("create_net_label: null or empty label string.");
        return (false);
    }

    if (check) {
        CDol *ol = find_net_labels(grp, string);
        if (ol) {
            CDol::destroy(ol);
            return (true);
        }
    }

    // Find a reference object.  We prefer Routing layers, but will
    // settle for a Conductor.
    CDo *od = 0;
    CDl *pld = 0;
    sGroup &g = gd_groups[grp];

    if (g.net()) {
        for (CDol *ol = g.net()->objlist(); ol; ol = ol->next) {
            CDo *odesc = ol->odesc;
            if (odesc->type() == CDBOX || odesc->type() == CDPOLYGON ||
                    odesc->type() == CDWIRE) {
                if (odesc->ldesc()->isRouting()) {
                    pld = EX()->getPinLayer(odesc->ldesc(), true);
                    if (!pld)
                        continue;
                    od = odesc;
                    break;
                }
            }
        }
        if (!od) {
            for (CDol *ol = g.net()->objlist(); ol; ol = ol->next) {
                CDo *odesc = ol->odesc;
                if (odesc->type() == CDBOX || odesc->type() == CDPOLYGON ||
                        odesc->type() == CDWIRE) {
                    if (odesc->ldesc()->isConductor()) {
                        pld = EX()->getPinLayer(odesc->ldesc(), true);
                        if (!pld)
                            continue;
                        od = odesc;
                        break;
                    }
                }
            }
        }
    }
    if (od) {
        int x, y;
        if (od->type() == CDBOX) {
            x = (od->oBB().left + od->oBB().right)/2;
            y = (od->oBB().bottom + od->oBB().top)/2;
        }
        else {
            x = y = 0;
            Zlist *zl = od->toZlist();
            Zlist *zx = 0;
            double a = 0;
            for (Zlist *z = zl; z; z = z->next) {
                double aa = z->Z.area();
                if (aa > a) {
                    a = aa;
                    zx = z;
                }
            }
            if (zx) {
                x = (zx->Z.xll + zx->Z.xul + zx->Z.xlr + zx->Z.xur)/4;
                y = (zx->Z.yl + zx->Z.yu)/2;
            }
        }
        Label label;
        label.label = new hyList(0, string, HYcvPlain); 
        label.x = x;
        label.y = y;
        label.xform = TXTF_HJC | TXTF_VJC;
        DSP()->DefaultLabelSize(string, Physical, &label.width, &label.height);
        CDla *la = gd_celldesc->newLabel(0, &label, pld, 0, false);
        if (!la) {
            Errs()->add_error("create_net_label: label creation failed.");
            return (false);
        }
        if (newp)
            *newp = la;
    }
    return (true);
}


// Return a list of the associated name labels found for the group
// whose number is passed.  If string is not empty or null, return
// only labels with matching text.  Otherwise, return all labels.
//
CDol *
cGroupDesc::find_net_labels(int grp, const char *string)
{
    if (grp < 1 || grp >= gd_asize)
        return (0);
    sGroup &g = gd_groups[grp];
    CDol *olist = 0;
    if (g.net()) {
        for (CDol *ol = g.net()->objlist(); ol; ol = ol->next) {
            CDo *odesc = ol->odesc;
            if (odesc->type() == CDBOX || odesc->type() == CDPOLYGON ||
                    odesc->type() == CDWIRE) {
                CDl *pld = EX()->getPinLayer(odesc->ldesc(), false);
                if (pld)
                    olist = find_net_labels(olist, odesc, pld, string);
                if (EX()->isFindOldTermLabels()) {
                    olist = find_net_labels(olist, odesc, odesc->ldesc(),
                        string);
                }
            }
        }
    }

    // If we find a label at the top level, we're done.  Otherwise,
    // look for labels in the top-level cell intersecting connected
    // nets in subcells.

    if (!olist) {
        cTfmStack stk;
        olist = find_net_labels_rc(gd_celldesc, stk, grp, string, 0);
    }
    return (olist);
}


// Look for net labels over odesc on pld, if found add to olist.  If
// string is not null or empty, ignore labels that don't match the
// string.
//
CDol *
cGroupDesc::find_net_labels(CDol *olist, const CDo *odesc, const CDl *pld,
    const char *string)
{
    CDg gdesc;
    gdesc.init_gen(gd_celldesc, pld, &odesc->oBB());
    CDo *od;
    while ((od = gdesc.next()) != 0) {
        if (od->type() != CDLABEL)
            continue;
        CDla *la = (CDla*)od;
        if (string && *string) {
            char *s = hyList::string(la->label(), HYcvPlain, true);
            if (!s || strcmp(s, string)) {
                delete [] s;
                continue;
            }
            delete [] s;
        }
        Point_c px(la->xpos(), la->ypos());
        if (odesc->intersect(&px, true)) {
            if (EX()->pinLayer() == pld) {
                // If there is a common pin layer, reject the match if
                // there is another object on a layer above which
                // would also match.

                bool skip = false;
                BBox BB(px.x-1, px.y-1, px.x+1, px.y+1);
                CDextLgen gen(CDL_CONDUCTOR, CDextLgen::TopToBot);
                CDl *ld;
                while (!skip && (ld = gen.next()) != 0) {
                    if (ld->physIndex() <= odesc->ldesc()->physIndex())
                        break;
                    CDg tgdesc;
                    tgdesc.init_gen(gd_celldesc, ld, &BB);
                    CDo *tod;
                    while ((tod = gdesc.next()) != 0) {
                        if (tod->type() == CDBOX || tod->type() == CDPOLYGON ||
                                tod->type() == CDWIRE) {
                            if (tod->intersect(&px, true)) {
                                skip = true;
                                break;
                            }
                        }
                    }
                }
                if (skip)
                    continue;
            }

            if (ExtErrLog.log_extracting() && ExtErrLog.verbose()) {
                char *s = hyList::string(la->label(), HYcvPlain, true);
                ExtErrLog.add_log(ExtLogExtV,
                    "Found net label %s on %s at %d,%d.", s,
                    la->ldesc()->name(), px.x, px.y);
                delete [] s;
            }
            olist = new CDol(od, olist);
        }
    }
    return (olist);
}


// Hunt for net name labels.  For nets that are unambiguously labeled,
// set the sGroup::netname.  Call this after grouping, the net names
// are used in association.
//
int
cGroupDesc::find_set_net_names()
{
    // This looks at all labels in the current cell and makes sure
    // that their reference point is associated (if there is one). 
    // This means adding subcircuit connections between hierarchy
    // levels and new groups, as needed.  Without this, the second
    // part of this function may not find these labels.
    //
    CDl *ld;
    CDextLgen lgen(CDL_CONDUCTOR);
    while ((ld = lgen.next()) != 0) {
        CDl *pld = EX()->getPinLayer(ld, false);
        if (!pld)
            continue;

        CDg gdesc;
        gdesc.init_gen(gd_celldesc, pld);
        CDo *od;
        while ((od = gdesc.next()) != 0) {
            if (od->type() != CDLABEL)
                continue;
            CDla *la = (CDla*)od;
            find_group_at_location(ld, la->xpos(), la->ypos(), true);
        }
    }

    // Now do the actual processing.  We look for labels only on
    // subcircuit groups that have a path to the top level.

    bool ignlab = EX()->isIgnoreNetLabels();
    int numfound = 0;
    for (int i = 1; i < gd_asize; i++) {
        gd_groups[i].set_netname(0, sGroup::NameFromNode);
        if (ignlab)
            continue;

        CDol *ol = find_net_labels(i, 0);

        // Make sure that the name is unique.
        CDnetName name = 0;
        for (CDol *o = ol; o; o = o->next) {
            // Can't assume that global names have already been added.
            char *s = hyList::string(((CDla*)o->odesc)->label(), HYcvPlain,
                true);
            CDnetName nn = CDnetex::name_tab_add(s);
            delete [] s;
            if (!nn)
                continue;
            if (!name) {
                name = nn;
                continue;
            }
            if (name != nn) {
                ExtErrLog.add_err(
                    "In %s, group %d has conflicting net labels\n"
                    "%s and %s, labels ignored.",
                    Tstring(gd_celldesc->cellname()), i,
                    Tstring(name), Tstring(nn));
                name = 0;
                break;
            }
        }
        CDol::destroy(ol);
        if (!name)
            continue;

        gd_groups[i].set_netname(name, sGroup::NameFromLabel);
        numfound++;
    }
    return (numfound);
}


// Return a group number that would apply for terminal or label
// matching at x,y to an object on layer ld, or any conducting object
// if ld is null.  This walks the hierarchy looking for geometry on
// nets that can reflect up to the calling context.  If map is true,
// virtual subcircuit contacts and groups will be added as needed. 
// Otherwise, matching will fail if there is no net connection to the
// top level.
//
int
cGroupDesc::find_group_at_location(const CDl *ld, int x, int y, bool map)
{
    if (ld && !ld->isConductor())
        return (-1);

    // Easy case, metal at top level.
    for (int i = 1; i < gd_asize; i++) {
        if (!gd_groups[i].net())
            continue;
        if (!gd_groups[i].net()->BB().intersect(x, y, true))
            continue;
        for (CDol *ol = gd_groups[i].net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isConductor())
                continue;
            if ((!ld || ld == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(x, y, true)) {
                return (i);
            }
        }
    }
    if (gd_groups[0].net() &&
            gd_groups[0].net()->BB().intersect(x, y, true)) {
        // Check ground group last.
        for (CDol *ol = gd_groups[0].net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isConductor())
                continue;
            if ((!ld || ld == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(x, y, true)) {
                return (0);
            }
        }
    }

    // No joy, recurse into subcircuits.
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (!s->cdesc()->oBB().intersect(x, y, true))
                continue;

            sSubcInstList *tsl = new sSubcInstList(s, 0);
            cTfmStack stk;
            int grp = find_group_at_location_rc(tsl, ld, x, y, stk, map);
            delete tsl;
            stk.TPop();
            if (grp >= 0)
                return (grp);
        }
    }
    return (-1);
}


// Private recursive tail for find_net_labels.
//
CDol *
cGroupDesc::find_net_labels_rc(const CDs *topsd, cTfmStack &stk, int grp,
    const char *string, CDol *olist)
{
    // On the first pass, we look at net geometry in the instances
    // connected to the net.

    CDol *olref = olist;
    for (sSubcContactList *scl = gd_groups[grp].subc_contacts(); scl;
            scl = scl->next()) {
        sSubcInst *si = scl->contact()->subc();
        CDc *cd = si->cdesc();
        CDs *msd = cd->masterCell();
        if (!msd)
            continue;
        cGroupDesc *gd = msd->groups();
        if (!gd)
            continue;
        sGroup *g = gd->group_for(scl->contact()->subc_group());
        if (!g || !g->net())
            continue;
        stk.TPush();
        stk.TApplyTransform(cd);
        stk.TPremultiply();
        if (si->ix() || si->iy()) {
            CDap ap(cd);
            stk.TTransMult(si->ix()*ap.dx, si->iy()*ap.dy);
        }

        for (CDol *ol = g->net()->objlist(); ol; ol = ol->next) {
            CDo *odesc = ol->odesc;
            if (odesc->type() == CDBOX || odesc->type() == CDPOLYGON ||
                    odesc->type() == CDWIRE) {
                CDl *pld = EX()->getPinLayer(odesc->ldesc(), false);
                if (!pld)
                    continue;

                odesc = odesc->copyObjectWithXform(&stk, false);
                CDg gdesc;
                gdesc.init_gen(topsd, pld, &odesc->oBB());
                CDo *od;
                while ((od = gdesc.next()) != 0) {
                    if (od->type() != CDLABEL)
                        continue;
                    CDla *la = (CDla*)od;
                    if (string && *string) {
                        char *s = hyList::string(la->label(), HYcvPlain, true);
                        if (!s || strcmp(s, string)) {
                            delete [] s;
                            continue;
                        }
                        delete [] s;
                    }
                    Point_c px(la->xpos(), la->ypos());
                    if (odesc->intersect(&px, true)) {
                        if (ExtErrLog.log_extracting() &&
                                ExtErrLog.verbose()) {
                            char *s = hyList::string(la->label(), HYcvPlain,
                                true);
                            ExtErrLog.add_log(ExtLogExtV,
                                "Found net label %s on %s at %d,%d.", s,
                                la->ldesc()->name(), px.x, px.y);
                            delete [] s;
                        }
                        olist = new CDol(od, olist);
                    }
                }
                delete odesc;
            }
        }
        stk.TPop();
    }
    if (olist != olref)
        return (olist);

    for (sSubcContactList *scl = gd_groups[grp].subc_contacts(); scl;
            scl = scl->next()) {
        sSubcInst *si = scl->contact()->subc();
        CDc *cd = si->cdesc();
        CDs *msd = cd->masterCell();
        if (!msd)
            continue;
        cGroupDesc *gd = msd->groups();
        if (!gd)
            continue;
        sGroup *g = gd->group_for(scl->contact()->subc_group());
        if (!g)
            continue;
        stk.TPush();
        stk.TApplyTransform(cd);
        stk.TPremultiply();
        if (si->ix() || si->iy()) {
            CDap ap(cd);
            stk.TTransMult(si->ix()*ap.dx, si->iy()*ap.dy);
        }
        olist = gd->find_net_labels_rc(topsd, stk,
            scl->contact()->subc_group(), string, olist);
        stk.TPop();
    }
    return (olist);
}


// Private recursive tail for find_group_at_location.
//
int
cGroupDesc::find_group_at_location_rc(const sSubcInstList *sil, const CDl *ld,
    int lx, int ly, cTfmStack &stk, bool map)
{
    CDc *cdesc = sil->inst()->cdesc();
    CDs *msd = cdesc->masterCell();
    if (!msd)
        return (-1);
    cGroupDesc *gd = msd->groups();
    if (!gd)
        return (-1);
    stk.TPush();
    stk.TApplyTransform(cdesc);
    stk.TPremultiply();
    CDap ap(cdesc);
    stk.TTransMult(sil->inst()->ix()*ap.dx, sil->inst()->iy()*ap.dy);

    int x = lx;
    int y = ly;
    stk.TInverse();
    stk.TInversePoint(&x, &y);

    // Check contacts that have a path to the top.
    for (sSubcContactInst *ci = sil->inst()->contacts(); ci; ci = ci->next()) {
        int topg = sil->top_group(ci->subc_group());
        if (topg < 0)
            continue;

        sGroup &g = gd->gd_groups[ci->subc_group()];
        if (!g.net() || !g.net()->BB().intersect(x, y, true))
            continue;
        for (CDol *ol = g.net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isConductor())
                continue;
            if ((!ld || ld == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(x, y, true)) {
                return (topg);
            }
        }
    }
    if (gd->gd_groups[0].net() &&
            gd->gd_groups[0].net()->BB().intersect(x, y, true)) {
        // Check ground group last.
        for (CDol *ol = gd->gd_groups[0].net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isConductor())
                continue;
            if ((!ld || ld == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(x, y, true)) {
                return (0);
            }
        }
    }

    // Recurse.
    for (sSubcList *sl = gd->gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (!s->cdesc()->oBB().intersect(x, y, true))
                continue;

            sSubcInstList *tsl = new sSubcInstList(s, sil);
            int grp = gd->find_group_at_location_rc(tsl, ld, lx, ly, stk, map);
            delete tsl;
            stk.TPop();
            if (grp >= 0)
                return (grp);
        }
    }

    // If not mapping, we found nothing and are done.
    if (!map)
        return (-1);

    // Look through all of the groups, if we find matching geometry we
    // will add a contact path to the top level.

    for (int i = 1; i < gd->gd_asize; i++) {
        sGroup &g = gd->gd_groups[i];
        if (!g.net() || !g.net()->BB().intersect(x, y, true))
            continue;
        for (CDol *ol = g.net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isConductor())
                continue;
            if ((!ld || ld == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(x, y, true)) {

                // Good, found an object.  We now walk back to the
                // top, adding subcircuit connection links as we go.

                cGroupDesc *pg = this;
                const sSubcInstList *sl = sil;
                int sbg = i;
                for (;;) {
                    int group = sl->parent_group(sbg);
                    if (group < 0) {
                        // No existing link, make one.
                        group = pg->nextindex();
                        pg->alloc_groups(group + 1);

                        sSubcContactInst *ci = sl->inst()->add(group, sbg);
                        pg->gd_groups[group].set_subc_contacts(
                            new sSubcContactList(ci,
                            pg->gd_groups[group].subc_contacts()));

                        ExtErrLog.add_log(ExtLogExtV,
                            "New group %d in %s for link to %d in %s.",
                            group, Tstring(pg->gd_celldesc->cellname()),
                            sbg, Tstring(gd->gd_celldesc->cellname()));
                    }
                    sl = sl->next();
                    if (!sl)
                        return (group);
                    gd = pg;
                    CDs *parent = sl->inst()->cdesc()->parent();
                    if (!parent)
                        return (-1);  // can't happen
                    pg = parent->groups();
                    if (!pg)
                        return (-1);  // can't happen
                    sbg = group;
                }
            }
        }
    }
    return (-1);
}

