
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

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_ep_comp.h"
#include "ext_nets.h"
#include "ext_errlog.h"
#include "cd_netname.h"
#include "cd_celldb.h"
#include "sced.h"
#include "sced_nodemap.h"
#include "sced_param.h"
#include "spnumber/paramsub.h"
#include "spnumber/spnumber.h"

#define TIME_DBG
#ifdef TIME_DBG
#include "miscutil/timedbg.h"
#endif


/*========================================================================*
 *
 *  Functions for electrical/comparison of subcircuits, devices, nets.
 *
 *========================================================================*/


namespace {
    // Find and return the indices of the permutable terminals, if any.
    // No checking here, we assume that the database is valid.
    //
    void find_prm_indices(const sDevInst *di, int *p1, int *p2)
    {
        *p1 = -1;
        *p2 = -1;
        if (!di || !di->desc()->permute_cont1())
            return;
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
            if (ci->desc()->name() == di->desc()->permute_cont1())
                *p1 = ci->desc()->elec_index();
            else if (ci->desc()->name() == di->desc()->permute_cont2())
                *p2 = ci->desc()->elec_index();
        }
    }
}


// Compare the physical group and electrical net, return a value
// 0-CMP_SCALE indicating the degree of matching.  The optional
// sc_permuting is a pointer to a subcircuit that is in the
// permutation generator, so we want to match the contacts exactly.
//
int
cGroupDesc::ep_grp_comp(int grp, int node, const sSubcInst *sc_permuting)
{
    if (grp < 0 || node < 0)
        // shouldn't happen
        return (0);

    // If either the group or node is already associated, return 0 unless
    // they are associated to each other.
    int nchk = gd_groups[grp].node();
    int gchk = group_of_node(node);
    if (nchk == node && gchk == grp)
        return (CMP_SCALE);
    if (nchk >= 0 || gchk >= 0)
        return (0);

    sGroup *g = group_for(grp);
    if (!g)
        return (0);

    // Compare net names if we can find any.
    CDnetName gn = g->netname();
    if (gn) {
        if (gn == gd_etlist->net_name(node))
            return (CMP_SCALE);
        if (gd_etlist->find_node(gn) >= 0)
            return (0);
    }

    CDcterm **dev_terms = 0;
    CDcterm **subc_terms = 0;
    CDsterm **formal_terms = 0;
    int num_dev_terms = 0;
    int num_subc_terms = 0;
    int num_formal_terms = 0;

    CDpin *p0 = pins_of_node(node);
    for (CDpin *p = p0; p; p = p->next())
        num_formal_terms++;

    CDcont *t0 = conts_of_node(node);
    if (!t0 && !num_formal_terms)
        return (0);
    for (CDcont *t = t0; t; t = t->next()) {
        if (!t->term()->instance())
            continue;
        CDs *msdesc = t->term()->instance()->masterCell();
        if (!msdesc)
            continue;
        if (msdesc->isDevice())
            num_dev_terms++;
        else
            num_subc_terms++;
    }
    if (!num_dev_terms && !num_subc_terms) {
        // If the net is not connected to anything but a contact
        // terminal, and if the group is also disconnected, say it is
        // a match.

        if (g->net() && !g->subc_contacts() && !g->device_contacts())
            return (CMP_SCALE);
        else
            return (0);
    }
    if (num_formal_terms) {
        formal_terms = new CDsterm*[num_formal_terms];
        num_formal_terms = 0;
        for (CDpin *p = p0; p; p = p->next())
            formal_terms[num_formal_terms++] = p->term();
    }
    if (num_dev_terms) {
        dev_terms = new CDcterm*[num_dev_terms];
        num_dev_terms = 0;
    }
    if (num_subc_terms) {
        subc_terms = new CDcterm*[num_subc_terms];
        num_subc_terms = 0;
    }
    for (CDcont *t = t0; t; t = t->next()) {
        if (!t->term()->instance())
            continue;
        CDs *msdesc = t->term()->instance()->masterCell();
        if (!msdesc)
            continue;
        if (msdesc->isDevice())
            dev_terms[num_dev_terms++] = t->term();
        else
            subc_terms[num_subc_terms++] = t->term();
    }
    int cnt = num_formal_terms + num_dev_terms + num_subc_terms;
    int good = 0;

    // cnt will be the number of physical plus the number of electrical
    // terms.  good is the number of terms in common.

    int num_devs_left = num_dev_terms;
    int num_subc_left = num_subc_terms;
    int num_formal_left = num_formal_terms;

    for (sDevContactList *dc = g->device_contacts(); dc; dc = dc->next()) {
        sDevInst *di = dc->contact()->dev();
        int p1, p2;
        find_prm_indices(di, &p1, &p2);

        for (int i = 0; i < num_dev_terms; i++) {
            CDcterm *term = dev_terms[i];
            if (!term)
                continue;

            if (di->dual()) {
                if (di->dual()->cdesc() != term->instance())
                    continue;
            }
            else {
                if (di->desc()->name() != term->instance()->cellname())
                    continue;
            }

            // There is at most one set of permutable terminals
            // per device.

            int ix = term->index();
            if (ix == dc->contact()->desc()->elec_index()) {
                good++;
                num_devs_left--;
                dev_terms[i] = 0;
                break;
            }
            else if (ix == p1) {
                if (dc->contact()->desc()->elec_index() == p2) {
                    good++;
                    num_devs_left--;
                    dev_terms[i] = 0;
                    break;
                }
            }
            else if (ix == p2) {
                if (dc->contact()->desc()->elec_index() == p1) {
                    good++;
                    num_devs_left--;
                    dev_terms[i] = 0;
                    break;
                }
            }
        }
        cnt++;
    }

    // Count the number of matches involving a subcircuit group with a
    // label.  We give these a slightly higher score than unlabeled
    // groups so in the event of a conflict, the labels win.
    //
    int label_bias = 0;

    for (sSubcContactList *sc = g->subc_contacts(); sc; sc = sc->next()) {
        sSubcContactInst *ci = sc->contact();
        sSubcInst *subc = ci->subc();
        CDs *sd = subc->cdesc()->masterCell(true);
        cGroupDesc *gd = sd ? sd->groups() : 0;
        if (!gd)
            continue;

        for (int i = 0; i < num_subc_terms; i++) {
            CDcterm *term = subc_terms[i];
            if (!term)
                continue;

            if (sd->cellname() != term->instance()->cellname())
                continue;
            if (subc->dual() && subc->dual()->cdesc() != term->instance())
                continue;

            int subg = term->master_group();
            // This will fail unless we make sure that the vgroup is
            // always set in virtual terminals, even when not placed.

            if (subg < 0) {
                if (ci->is_wire_only() || ci->is_global()) {
                    // consistent
                    good++;
                    num_subc_left--;
                    subc_terms[i] = 0;
                    break;
                }
            }
            else if (subc == sc_permuting) {
                // The subc is being permuted, require an exact match.
                if (subg == ci->subc_group()) {
                    sGroup *gp = gd->group_for(subg);
                    if (gp && gp->netname() &&
                            gp->netname_origin() == sGroup::NameFromLabel)
                        label_bias++;
                    good++;
                    num_subc_left--;
                    subc_terms[i] = 0;
                    break;
                }
            }
            else {
                // Otherwise accept any permutation.
                if (subg == ci->subc_group() || (subc->permutes() &&
                        subc->permutes()->is_equiv(subg, ci->subc_group()))) {
                    sGroup *gp = gd->group_for(subg);
                    if (gp && gp->netname() &&
                            gp->netname_origin() == sGroup::NameFromLabel)
                        label_bias++;
                    good++;
                    num_subc_left--;
                    subc_terms[i] = 0;
                    break;
                }
            }
        }
        cnt++;
    }

    // Now account for formal terminal connections.
    for (CDpin *p = g->termlist(); p; p = p->next()) {
        for (int i = 0; i < num_formal_terms; i++) {
            if (formal_terms[i] == p->term()) {
                num_formal_left--;
                good++;
                formal_terms[i] = 0;
                break;
            }
        }
        cnt++;
    }

    // If only formal terminals left, count those that aren't yet
    // assigned to a group as a match.

    if (num_formal_left && !num_devs_left && !num_subc_left) {
        for (int i = 0; i < num_formal_terms; i++) {
            CDsterm *term = formal_terms[i];
            if (!term)
                continue;
            int gnum = term->group();
            if (gnum < 0 || gnum == grp) {
                good++;
                cnt++;
            }
        }
    }

    delete [] formal_terms;
    delete [] dev_terms;
    delete [] subc_terms;

    int del = 0;
    if (good*4 > cnt) {
        // More tests, mostly tweeks to avoid symmetry breaking.

        // If the electrical info was obtained from physical data,
        // the assigned node name is 'nGNUM' where GNUM is the group
        // number.  Here, we check for this and if consistency is
        // found, the score is incremented.  This should help ensure
        // convergence in such cases.

        const char *nn = Tstring(gd_etlist->net_name(node));
        if (nn && nn[0] == 'n' && isdigit(nn[1])) {
            if (grp == atoi(nn+1))
                del++;
        }

        // It the node matched the hint node, increment slightly. 
        // This will break symmetry in favor of matching the
        // schematic.

        if (gd_groups[grp].hint_node() == node)
            del += 10;

        // Add the labeled subcircuit contact bias here.
        del += 10*label_bias;

        // Score from matching of hierarchy net names.
        del += ep_hier_comp(grp, node);
    }

    // If perfect matching, good/cnt = 1/2.
    return (del + (2*CMP_SCALE*good)/cnt);
}


// Attempt to resolve an unresolved bulk contact.
//
void
cGroupDesc::check_bulk_contact(const sDevInst *di, sDevContactInst *ci)
{
    if (ci->group() < 0) {
        sDevContactDesc *cd = ci->desc();
        if (cd->level() == BC_skip || cd->level() == BC_defer) {
            CDnetName nm = cd->netname();
            // This should be the name of a global net, attempt to set
            // the group number.

            CDs *ecd = CDcdb()->findCell(gd_celldesc->cellname(),
                Electrical);
            cNodeMap *map = ecd ? ecd->nodes() : 0;
            if (nm && map && SCD()->isGlobalNetName(Tstring(nm))) {
                int grp = group_of_node(map->findNode(nm));
                if (grp < 0) {
                    for (int i = 0; i < gd_asize; i++) {
                        if (gd_groups[i].netname() == nm) {
                            grp = i;
                            break;
                        }
                    }
                }
                if (grp >= 0) {
                    ExtErrLog.add_log(ExtLogAssocV,
                        "resolved %s %d bulk contact group %d by name %s",
                        Tstring(di->desc()->name()), di->index(), grp,
                        cd->netname_gvn());
                    ci->set_group(grp);
                    link_contact(ci);
                }
            }
        }
    }
}


// Use the extracted hierarchy to fine-tune group/node association
// scores.  This will use the instance placements to break symmetry.
//
int
cGroupDesc::ep_hier_comp(int grp, int node)
{
    // Both grp and node must be cell connections.
    sGroup *g = group_for(grp);
    if (!g)
        return (0);
    if (!g->cell_connection())
        return (0);

    CDp_snode *ps = 0;
    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    if (esdesc) {
        ps = (CDp_snode*)esdesc->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            if (ps->enode() == node)
                break;
        }
    }
    if (!ps || !ps->cell_terminal())
        return (0);

    sGdList *gdl0 = EX()->referenceList(gd_celldesc);
    int score = 0;

    if (!g->netname()) {
        for (sGdList *gdl = gdl0; gdl; gdl = gdl->next) {
            cGroupDesc *pgd = gdl->gd;
            CDs *pesd = CDcdb()->findCell(pgd->gd_celldesc->cellname(),
                Electrical);
            cNodeMap *pmap = pesd ? pesd->nodes() : 0;
            if (!pmap)
                continue;
            sSubcList *sl = pgd->subckts();
            for ( ; sl; sl = sl->next()) {
                sSubcInst *si = sl->subs();
                if (si && si->cdesc()->cellname() == gd_celldesc->cellname())
                    break;
            }
            if (!sl)
                continue;

            // Look at the groups in parent cells that connect to our
            // group at an instance.  Collect the net name label text
            // in tab.

            SymTab tab(false, false);
            for (sSubcInst *si = sl->subs(); si; si = si->next()) {
                sSubcContactInst *ci = si->contacts();
                for ( ; ci; ci = ci->next()) {
                    if (ci->subc_group() == grp)
                        break;
                }
                if (!ci)
                    continue;

                sGroup *pg = pgd->group_for(ci->parent_group());
                if (pg && pg->netname() &&
                        pg->netname_origin() >= sGroup::NameFromTerm) {
                    if (SymTab::get(&tab, (unsigned long)pg->netname()) ==
                            ST_NIL)
                        tab.add((unsigned long)pg->netname(), 0, false);
                }
            }
            if (!tab.allocated())
                continue;

            // Do the same maneuver for the electrical cell and compare net
            // names found.
            for (sEinstList *el = sl->esubs(); el; el = el->next()) {
                unsigned int vix = el->cdesc_index();
                CDp_cnode *pc;
                if (vix > 0) {
                    CDp_range *pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
                    if (!pr)
                        continue;
                    pc = pr->node(0, vix, ps->index());
                }
                else {
                    pc = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
                    for ( ; pc; pc = pc->next()) {
                        if (pc->index() == ps->index())
                            break;
                    }
                }
                if (!pc)
                    continue;
                CDnetName nn = pmap->mapStab(pc->enode());
                if (SymTab::get(&tab, (unsigned long)nn) == ST_NIL)
                    score--;
                else
                    score++;
            }
        }
    }
 
    score += ep_hier_comp_rc(grp, node);
    return (score);
}


// This test will walk up the hierarchy recursively looking for an
// association.  This is the recursive tail of ep_hier_comp.
//
int
cGroupDesc::ep_hier_comp_rc(int grp, int node)
{
    if (first_pass())
        return (0);
    sGroup *g = group_for(grp);
    if (!g)
        return (0);
    if (g->node() >= 0)
        return (g->node() == node ? 1 : -100);

    CDp_snode *ps = 0;
    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    cNodeMap *map = esdesc->nodes();
    if (esdesc) {
        ps = (CDp_snode*)esdesc->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            if (ps->enode() == node)
                break;
        }
    }
    if (ps && ps->cell_terminal()) {
        if (g->netname() == ps->cell_terminal()->name())
            return (1);

        if (g->netname()) {
            if (map && map->findNode(g->netname()) == node)
                return (1);
            return (-100);
        }
    }
    else
        return (0);
    if (!g->cell_connection())
        return (0);

    sGdList *gdl0 = EX()->referenceList(gd_celldesc);
    for (sGdList *gdl = gdl0; gdl; gdl = gdl->next) {
        cGroupDesc *pgd = gdl->gd;
        sSubcList *sl = pgd->subckts();
        for ( ; sl; sl = sl->next()) {
            sSubcInst *si = sl->subs();
            if (!si)
                continue;
            if (si->cdesc()->cellname() == gd_celldesc->cellname())
                break;
        }
        if (!sl)
            continue;
        SymTab ptab(false, false);
        for (sSubcInst *si = sl->subs(); si; si = si->next()) {
            sSubcContactInst *ci = si->contacts();
            for ( ; ci; ci = ci->next()) {
                if (ci->subc_group() == grp)
                    break;
            };
            if (!ci || ci->parent_group() < 0)
                continue;

            if (si->dual()) {
                sEinstList *el = si->dual();
                CDp_cnode *pc;
                if (el->cdesc_index() == 0) {
                    pc = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
                    for ( ; pc; pc = pc->next()) {
                        if (pc->index() == ps->index())
                            break;
                    }
                }
                else {
                    CDp_range *pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
                    if (pr)
                        pc = pr->node(0, el->cdesc_index(), ps->index());
                    else
                        pc = 0;
                }
                if (pc) {
                    int ret = pgd->ep_hier_comp_rc(ci->parent_group(),
                        pc->enode());
                    if (ret > 0)
                        return (ret);
                }
                continue;
            }

            if (SymTab::get(&ptab, ci->parent_group()) != ST_NIL)
                continue;
            ptab.add(ci->parent_group(), 0, false);
        }
        if (ptab.allocated() > 0) {

            SymTab etab(false, false);
            for (sEinstList *el = sl->esubs(); el; el = el->next()) {
                CDp_cnode *pc;
                if (el->cdesc_index() == 0) {
                    pc = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
                    for ( ; pc; pc = pc->next()) {
                        if (pc->index() == ps->index())
                            break;
                    }
                }
                else {
                    CDp_range *pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
                    if (!pr)
                        continue;
                    pc = pr->node(0, el->cdesc_index(), ps->index());
                }
                if (pc && SymTab::get(&etab, pc->enode()) == ST_NIL)
                    etab.add(pc->enode(), 0, false);
            }
            SymTabGen pgen(&ptab);
            SymTabEnt *pent, *eent;
            while ((pent = pgen.next()) != 0) {
                int tg = (unsigned long)pent->stTag;
                SymTabGen egen(&etab);
                while ((eent = egen.next()) != 0) {
                    int tn = (unsigned long)eent->stTag;
                    int ret = pgd->ep_hier_comp_rc(tg, tn);
                    if (ret > 0)
                        return (ret);
                }
            }
        }
    }

    return (0);
}


// Compare the extracted parameters of the physical and electrical
// devices.  Return positive if the parameters match, 0 if the test is
// inconclusive, negative if the parameters differ.
//
// Side effect:  This will permute the terminal names to maximize
// the score.
//
int
cGroupDesc::ep_param_comp(sDevInst *di, const sEinstList *el) THROW_XIrt
{
    bool do_comp = false;
    for (sMeasure *m = di->desc()->measures(); m; m = m->next()) {
        if (m->lvsword()) {
            do_comp = true;
            break;
        }
    }
    if (!do_comp)
        return (0);

    sParamTab *ptab;
    double *leadval;
    el->setup_eval(&ptab, &leadval);

    // Even if the parameter string is null, we still do comparison as
    // default parameter values can come from the parameter context.

    try {
        int cmp = 0;
        int tcmp = ep_param_comp_core(di, leadval, ptab);
        cmp = tcmp;
        if (di->permute_params()) {
            tcmp = ep_param_comp_core(di, leadval, ptab);
            if (tcmp > cmp)
                cmp = tcmp;
            else
                di->permute_params();
        }
        delete ptab;
        return (cmp);
    }
    catch (XIrt) {
        delete ptab;
        throw;
    }
}


// Compare extracted parameters to the relevant part of the SPICE
// line which appears in line.  Return positive if the parameters match,
// 0 if the test is inconclusive, negative if the parameters differ.
//
int
cGroupDesc::ep_param_comp_core(sDevInst *di, const double *leadval,
    sParamTab *ptab) THROW_XIrt
{
    int tot = 0;
    int cnt = 0;
#ifdef TIME_DBG
    Tdbg()->start_timing("measure_devices");
#endif
    bool ret = di->measure();
#ifdef TIME_DBG
    Tdbg()->accum_timing("measure_devices");
#endif
    if (!ret)
        throw (XIbad);

    for (sMeasure *m = di->desc()->measures(); m; m = m->next()) {
        if (m->lvsword()) {
            cnt++;
            double a = m->measure();
            const double *d = 0;
            if (!*m->lvsword()) {
                // Empty lvsword, this will match the leadval.
                d = leadval;
            }
            else {
                // The lvsword is either a parameter name, or a
                // single-quoted expression involving constants and
                // parameter names.

                char *string = lstring::copy(m->lvsword());
                if (ptab)
                    ptab->param_subst_all_collapse(&string);
                if (EX()->paramCx())
                    EX()->paramCx()->update(&string);

                // If substitution and evaluation succeeded, the
                // string should now consist of a single number.
                const char *t = string;
                d = SPnum.parse(&t, true);
                delete [] string;
            }
            if (d) {
                double u = fabs(a - *d);
                if (u == 0)
                    tot += CMP_SCALE;
                else {
                    double v = 0.5*(fabs(a) + fabs(*d));
                    for (int i = 0; i < m->precision(); i++)
                        v /= 10.0;

                    if (u <= v)
                        tot += CMP_SCALE;
                    else {
                        v *= 10.0;
                        if (u <= v)
                            tot += CMP_SCALE/2;
                        else
                            tot -= CMP_SCALE;
                    }
                }
            }
        }
    }
    if (!cnt)
        return (0);
    return (tot/cnt);
}


// Compare the physical and electrical subcircuits, return -1 -
// CMP_SCALE indicating the degree of matching.  At least one
// connected group must be associated or 0 is returned.  If there is
// an explicit mismatch, -1 is returned, unless allow_errs is set. 
// Otherwise the score represents the number of matches out of the
// number of connections.
//
int
cGroupDesc::ep_subc_comp(sSubcInst *su, const sEinstList *el, bool perm_fix)
{
    CDc *edesc = el->cdesc();
    unsigned int vecix = el->cdesc_index();

    CDs *msd = su->cdesc()->masterCell();
    cGroupDesc *gd = msd ? msd->groups() : 0;

    int score = 0;
    sExtPermGen<int> gen(su->permutes());
    while (gen.next()) {

        int tot = 0, cnt = 0, nmatch = 0;
        if (perm_fix) {
            // This mode is used when permuting nodes that have been
            // arbitrarily assigned from symmetry breaking to match
            // the parent cell connections.  All nodes have been
            // re-associated, subcircuits are not associated
            // (correctly).  The subcircuits may require the
            // permutation fix.  We match subcircuits by the
            // un-ordered node signatures, which will be identical
            // between physical and electrical.

            for (sSubcContactInst *ci = su->contacts(); ci; ci = ci->next()) {
                // Shouldn't happen, ignore.
                if (ci->parent_group() < 0 || ci->parent_group() >= gd_asize)
                    continue;
                if (gd) {
                    sGroup *g = gd->group_for(ci->subc_group());
                    if (g && (g->global() || g->unas_wire_only()))
                        continue;
                }
                cnt++;
            }
            int *ary = new int[cnt];
            cnt = 0;
            for (sSubcContactInst *ci = su->contacts(); ci; ci = ci->next()) {
                // Shouldn't happen, ignore.
                if (ci->parent_group() < 0 || ci->parent_group() >= gd_asize)
                    continue;
                if (gd) {
                    sGroup *g = gd->group_for(ci->subc_group());
                    if (g && (g->global() || g->unas_wire_only()))
                        continue;
                }
                ary[cnt++] = gd_groups[ci->parent_group()].node();
            }

            CDp_range *pr = (CDp_range*)edesc->prpty(P_RANGE);
            CDp_cnode *pcx = (CDp_cnode*)edesc->prpty(P_NODE);
            for ( ; pcx; pcx = pcx->next()) {
                CDp_cnode *pc = pcx;
                if (vecix > 0 && pr)
                    pc = pr->node(0, vecix, pcx->index());
                for (int i = 0; i < cnt; i++) {
                    if (pc->enode() == ary[i]) {
                        ary[i] = -CMP_SCALE;  // code for a match
                        break;
                    }
                }
            }
            for (int i = 0; i < cnt; i++) {
                if (ary[i] == -CMP_SCALE) {
                    nmatch++;
                    tot += CMP_SCALE;
                }
                else if (ary[i] >= 0) {
                    if (!allow_errs())
                        return (-1);
                    tot -= CMP_SCALE/10;
                }
            }
            delete [] ary;
        }
        else {
            for (sSubcContactInst *ci = su->contacts(); ci; ci = ci->next()) {
                // Shouldn't happen, ignore.
                if (ci->parent_group() < 0 || ci->parent_group() >= gd_asize)
                    continue;

                int node = ci->node(el);
                if (node < 0) {
                    // If the subcircuit connection is to an unassociated wire
                    // group, ignore this (i.e., don't increment cnt).

                    if (gd) {
                        sGroup *g = gd->group_for(ci->subc_group());
                        if (g && (g->global() || g->unas_wire_only()))
                            continue;
                    }
                }
                else {
                    int nci = gd_groups[ci->parent_group()].node();
                    if (nci < 0)
                        tot += ep_grp_comp(ci->parent_group(), node, su);
                    else if (nci == node) {
                        tot += CMP_SCALE;
                        nmatch++;
                    }
                    else {
                        int subg = ci->subc_group();
                        if (!su->permutes() ||
                                su->permutes()->num_states() <= 1 ||
                                !su->permutes()->is_permute(subg)) {
                            if (!allow_errs())
                                return (-1);
                            tot -= CMP_SCALE/10;
                        }
                    }
                }
                cnt++;
            }
        }

        int sc = 0;
        if (nmatch == 0) {
            // If cnt == 0, there were no ambiguous terminals, score a
            // match.
            if (cnt == 0)
                sc = CMP_SCALE;

            // If there were only two terminals and these were
            // connected backwards, score a match.
            else if (cnt == 2 && !su->permutes()) {
                int n1g = -1;
                int n2g = -1;
                int n1c = -1;
                int n2c = -1;
                int xx = 0;
                for (sSubcContactInst *ci = su->contacts(); ci;
                        ci = ci->next()) {
                    // Shouldn't happen, ignore.
                    if (ci->parent_group() < 0 ||
                            ci->parent_group() >= gd_asize)
                        continue;

                    int node = ci->node(el);
                    if (node < 0) {
                        if (gd) {
                            sGroup *g = gd->group_for(ci->subc_group());
                            if (g && (g->global() || g->unas_wire_only()))
                                continue;
                        }
                    }
                    else {
                        if (!xx) {
                            n1g = gd_groups[ci->parent_group()].node();
                            n1c = node;
                            xx++;
                        }
                        else {
                            n2g = gd_groups[ci->parent_group()].node();
                            n2c = node;
                        }
                    }
                }
                if (n1g >= 0 && n2g >= 0 && n1c >= 0 && n2c >= 0) {
                    if (n1g == n2c && n1c == n2g) {
                        // Terminals are connected backwards!  This is
                        // clear a matching device, so score it.
                        sc = CMP_SCALE;
                    }
                }
            }

            // If there was no contact checking, return a score that
            // will trigger symmetry breaking.
            else if (no_cont_brksym())
                sc = CMP_SCALE/2;
        }
        else
            sc = tot/cnt;
        if (sc > score)
            score = sc;
    }
    return (score);
}
// End of cGroupDesc functions.


//
// The sDevComp implements electrical/physical device comparison.
//

bool
sDevComp::set(sDevInst *di)
{
    if (!di) {
        Errs()->add_error(
            "Internal error: in sDevComp::set, null phys device address.");
        return (false);
    }
    dc_pdev = di;
    const sDevDesc *dd = di->desc();

    unsigned int numconts = dd->num_contacts();
    if (!numconts) {
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next())
            numconts++;
        if (!numconts) {
            Errs()->add_error("Instance of device %s has no contacts.",
                Tstring(di->desc()->name()));
            dc_conts_sz = 0;
            return (false);
        }
    }
    if (numconts > dc_conts_sz) {
        dc_conts_sz = numconts;
        delete [] dc_conts;
        dc_conts = new sDevContactInst*[dc_conts_sz];
    }
    memset(dc_conts, 0, dc_conts_sz*sizeof(sDevContactInst*));

    dc_gix = -1;
    sDevContactInst *ci = di->contacts();
    for ( ; ci; ci = ci->next()) {
        unsigned int ix = ci->desc()->elec_index();
        if (ix >= dc_conts_sz) {
            Errs()->add_error(
                "Instance of device %s, contact %s, index %d out "
                "of range.", Tstring(di->desc()->name()),
                Tstring(ci->desc()->name()), ix);
            dc_conts_sz = 0;
            return (false);
        }
        dc_conts[ix] = ci;

        // If a MOS device, save the gate index.
        if (di->desc()->is_mos()) {
            char c = *Tstring(ci->desc()->name());
            if (c == 'g' || c == 'G')
                dc_gix = ix;
        }
    }
    for (unsigned int i = 0; i < dc_conts_sz; i++) {
        if (!dc_conts[i]) {
            Errs()->add_error(
                "Instance of device %s, contact index %d unset.",
                Tstring(di->desc()->name()), i);
            dc_conts_sz = 0;
            return (false);
        }
    }
    dc_pix1 = -1;
    dc_pix2 = -1;
    if (dd->permute_cont1()) {
        for (ci = di->contacts(); ci; ci = ci->next()) {
            if (ci->desc()->name() == dd->permute_cont1())
                dc_pix1 = ci->desc()->elec_index();
            else if (ci->desc()->name() == dd->permute_cont2())
                dc_pix2 = ci->desc()->elec_index();
        }
        if (dc_pix1 < 0) {
            Errs()->add_error(
                "Instance of device %s, permute name %s not found.",
                Tstring(di->desc()->name()), Tstring(dd->permute_cont1()));
            dc_conts_sz = 0;
            return (false);
        }
        if (dc_pix2 < 0) {
            Errs()->add_error(
                "Instance of device %s, permute name %s not found.",
                Tstring(di->desc()->name()), Tstring(dd->permute_cont2()));
            dc_conts_sz = 0;
            return (false);
        }
    }
    return (true);
}


bool
sDevComp::set(sEinstList *el)
{
    if (!el) {
        Errs()->add_error(
            "Internal error: in sDevComp::set, null elec instance address.");
        return (false);
    }
    dc_edev = el;
    CDp_cnode *pc0 = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
    unsigned int cnt = 0;
    for (CDp_cnode *pc = pc0; pc; pc = pc->next(), cnt++) ;
    if (!cnt) {
        Errs()->add_error("Instance of device %s has no nodes.",
            Tstring(el->cdesc()->cellname()));
        return (false);
    }
    if (cnt > dc_nodes_sz) {
        dc_nodes_sz = cnt;
        delete [] dc_nodes;
        dc_nodes = new CDp_cnode*[dc_nodes_sz];
    }
    memset(dc_nodes, 0, dc_nodes_sz*sizeof(CDp_cnode*));
    unsigned int vix = el->cdesc_index();
    CDp_range *pr = 0;
    if (vix) {
        pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
        if (!pr) {
            Errs()->add_error("Instance of device %s has nonzero vector "
                "index but no range property.",
                Tstring(el->cdesc()->cellname()));
            return (false);
        }
    }
    for ( ; pc0; pc0 = pc0->next()) {
        CDp_cnode *pc;
        if (vix)
            pc = pr->node(0, vix, pc0->index());
        else
            pc = pc0;
        dc_nodes[pc0->index()] = pc;
    }
    for (unsigned int i = 0; i < dc_nodes_sz; i++) {
        if (!dc_nodes[i]) {
            Errs()->add_error(
                "Instance of device %s has no node property "
                "for index %d",
                Tstring(el->cdesc()->cellname()), i);
            return (false);
        }
    }
    return (true);
}


int
sDevComp::score(cGroupDesc *gd)
{
    if (!dc_conts_sz)
        return (-CMP_SCALE); // Bogus device, some error ocurred.

    int tot = 0, cnt = 0, nmatch = 0;
    bool didp = false;

    // First pass, just do the simple tests so we can bail early
    // on inconsistency.
    //
    for (int i = 0; i < (int)dc_conts_sz; i++) {
        if (i == dc_pix1 || i == dc_pix2) {
            if (!didp) {
                didp = true;
                sDevContactInst *c1 = dc_conts[dc_pix1];
                sDevContactInst *c2 = dc_conts[dc_pix2];
                int node1 = dc_nodes[dc_pix1]->enode();
                int node2 = dc_nodes[dc_pix2]->enode();
                if (node1 >= 0 && node2 >= 0) {
                    sGroup *g1 = gd->group_for(c1->group());
                    sGroup *g2 = gd->group_for(c2->group());
                    int nc1 = g1 ? g1->node() : -1;
                    int nc2 = g1 ? g2->node() : -1;
                    if (nc1 >= 0 && nc2 >= 0) {
                        if (nc1 == node1 && nc2 == node2) {
                            tot += 2*CMP_SCALE;
                            nmatch += 2;
                        }
                        else if (nc1 == node2 && nc2 == node1) {
                            tot += 2*CMP_SCALE;
                            nmatch += 2;
                        }
                        else
                            return (0);
                    }
                    else if (nc1 >= 0) {
                        if (nc1 == node1) {
                            tot += CMP_SCALE;
                            nmatch++;
                        }
                        else if (nc1 == node2) {
                            tot += CMP_SCALE;
                            nmatch++;
                        }
                        else
                            return (0);
                    }
                    else if (nc2 >= 0) {
                        if (nc2 == node1) {
                            tot += CMP_SCALE;
                            nmatch++;
                        }
                        else if (nc2 == node2) {
                            tot += CMP_SCALE;
                            nmatch++;
                        }
                        else
                            return (0);
                    }
                }
            }
        }
        else {
            sDevContactInst *ci = dc_conts[i];
            if (ci->desc()->is_bulk()) {
                gd->check_bulk_contact(dc_pdev, ci);
                continue;
            }
            int node = dc_nodes[i]->enode();
            if (node >= 0) {
                sGroup *g = gd->group_for(ci->group());
                int nc = g ? g->node() : -1;
                if (nc < 0)
                    ;
                else if (nc == node) {
                    tot += CMP_SCALE;
                    nmatch++;
                }
                else
                    return (0);
            }
        }
        cnt++;
    }

    if (nmatch < cnt) {
        // There were unmatched contacts, need a second pass to do
        // the expensive group/node comparisons.

        didp = false;
        for (int i = 0; i < (int)dc_conts_sz; i++) {
            if (i == dc_pix1 || i == dc_pix2) {
                if (!didp) {
                    didp = true;
                    sDevContactInst *c1 = dc_conts[dc_pix1];
                    sDevContactInst *c2 = dc_conts[dc_pix2];
                    int node1 = dc_nodes[dc_pix1]->enode();
                    int node2 = dc_nodes[dc_pix2]->enode();
                    if (node1 >= 0 && node2 >= 0) {
                        sGroup *g1 = gd->group_for(c1->group());
                        sGroup *g2 = gd->group_for(c2->group());
                        int nc1 = g1 ? g1->node() : -1;
                        int nc2 = g1 ? g2->node() : -1;
                        if (nc1 >= 0 && nc2 >= 0)
                            continue;
                        if (nc1 >= 0) {
                            if (nc1 == node1)
                                tot += gd->ep_grp_comp(c2->group(), node2);
                            else if (nc1 == node2)
                                tot += gd->ep_grp_comp(c2->group(), node1);
                        }
                        else if (nc2 >= 0) {
                            if (nc2 == node1)
                                tot += gd->ep_grp_comp(c1->group(), node2);
                            else if (nc2 == node2)
                                tot += gd->ep_grp_comp(c1->group(), node1);
                        }
                        else {
                            int x1 = gd->ep_grp_comp(c1->group(), node1);
                            int x2 = gd->ep_grp_comp(c1->group(), node2);
                            int y1 = gd->ep_grp_comp(c2->group(), node2);
                            int y2 = gd->ep_grp_comp(c2->group(), node1);
                            if (x1 + y1 >= x2 + y2)
                                tot += x1 + y1;
                            else
                                tot += x2 + y2;
                        }
                    }
                }
            }
            else {
                sDevContactInst *ci = dc_conts[i];
                if (ci->desc()->is_bulk())
                    continue;
                int node = dc_nodes[i]->enode();
                if (node >= 0) {
                    sGroup *g = gd->group_for(ci->group());
                    int nc = g ? g->node() : -1;
                    if (nc < 0)
                        tot += gd->ep_grp_comp(ci->group(), node);
                }
            }
        }
    }
    if (nmatch == 0) {
        // If cnt == 0, there were no ambiguous terminals, score a
        // match (makes no sense for devices).

        if (cnt == 0)
            return (CMP_SCALE);

        // If there was no contact checking, return a score that
        // will trigger symmetry breaking.

        if (gd->no_cont_brksym())
            return (CMP_SCALE/2);
    }
    return (tot/cnt);
}


void
sDevComp::associate(cGroupDesc *gd)
{
    for (int i = 0; i < (int)dc_conts_sz; i++) {
        int grp = dc_conts[i]->group();
        sGroup *g = gd->group_for(grp);
        if (!g || g->node() >= 0)
            continue;
        if (i == dc_pix1) {
            sDevContactInst *c2 = dc_conts[dc_pix2];
            sGroup *g2 = gd->group_for(c2->group());
            if (g2) {
                int n2 = g2->node();
                if (n2 >= 0) {
                    // The c2 group is associated to n2.

                    int node1 = dc_nodes[dc_pix1]->enode();
                    int node2 = dc_nodes[dc_pix2]->enode();
                    if (node1 >= 0 && node2 == n2) {
                        int gchk = gd->group_of_node(node1);
                        if (gchk < 0 || gchk == grp)
                            gd->set_association(grp, node1);
                    }
                    else if (node2 >= 0 && node1 == n2) {
                        int gchk = gd->group_of_node(node2);
                        if (gchk < 0 || gchk == grp)
                            gd->set_association(grp, node2);
                    }
                }
            }
        }
        else if (i == dc_pix2) {
            sDevContactInst *c1 = dc_conts[dc_pix1];
            sGroup *g1 = gd->group_for(c1->group());
            if (g1) {
                int n1 = g1->node();
                if (n1 >= 0) {
                    // The c1 group is associated to n1.

                    int node1 = dc_nodes[dc_pix1]->enode();
                    int node2 = dc_nodes[dc_pix2]->enode();
                    if (node1 >= 0 && node2 == n1) {
                        int gchk = gd->group_of_node(node1);
                        if (gchk < 0 || gchk == grp)
                            gd->set_association(grp, node1);
                    }
                    else if (node2 >= 0 && node1 == n1) {
                        int gchk = gd->group_of_node(node2);
                        if (gchk < 0 || gchk == grp)
                            gd->set_association(grp, node2);
                    }
                }
            }
        }
        else {
            int node = dc_nodes[i]->enode();
            if (node >= 0) {
                int gchk = gd->group_of_node(node);
                if (gchk < 0 || gchk == grp)
                    gd->set_association(grp, node);
            }
        }
    }
}


// Return true if the two electrical device instances are
// connected in parallel, accounting for permutations.
//
bool
sDevComp::is_parallel(const sEinstList *el)
{
    if (!el || !dc_edev)
        return (false);
    if (el->cdesc()->masterCell() != dc_edev->cdesc()->masterCell())
        return (false);

    unsigned int cnt = 0;
    unsigned int vix = el->cdesc_index();
    CDp_cnode *pc0 = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
    CDp_range *pr = 0;
    if (vix) {
        pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
        if (!pr)
            return (false);
    }
    int n1 = -1, n2 = -1;
    for ( ; pc0; pc0 = pc0->next()) {
        CDp_cnode *pc;
        if (vix)
            pc = pr->node(0, vix, pc0->index());
        else
            pc = pc0;

        if (pc->index() == (unsigned int)dc_pix1)
            n1 = pc->enode();
        else if (pc->index() == (unsigned int)dc_pix2)
            n2 = pc->enode();
        else {
            if (pc->enode() != dc_nodes[pc->index()]->enode())
                return (false);
        }
        cnt++;
    }
    if (cnt != dc_nodes_sz)
        return (false);
    if (n1 >= 0) {
        return (
            (n1 == dc_nodes[dc_pix1]->enode() &&
                n2 == dc_nodes[dc_pix2]->enode()) ||
            (n1 == dc_nodes[dc_pix2]->enode() &&
                n2 == dc_nodes[dc_pix1]->enode()));
    }
    return (true);
}


namespace {
    int gate_node(const CDc *cd, unsigned int vix, int gate_index)
    {
        CDp_cnode *pc0 = (CDp_cnode*)cd->prpty(P_NODE);
        CDp_range *pr = 0;
        if (vix > 0) {
            pr = (CDp_range*)cd->prpty(P_RANGE);
            if (!pr)
                return (-1);
        }
        for ( ; pc0; pc0 = pc0->next()) {
            CDp_cnode *pc;
            if (vix > 0)
                pc = pr->node(0, vix, pc0->index());
            else
                pc = pc0;
            if (pc->index() == (unsigned int)gate_index)
                return (pc->enode());
        }
        return (-1);
    }
}


// Return true if the two devices are permutable totem-pole ends. 
// These would be the end devices connected to the gate output, of
// parallel-connected totem poles.  The parallel-connected totem poles
// are permutable.  This function detects this case in device
// identification, avoiding time-consuming symmetry trials.
//
// This function can make a huge difference, reducing association time
// by a factor of 30 for the global_row_decoder test cell.
//
bool
sDevComp::is_mos_tpeq(cGroupDesc *gd, const sEinstList *el)
{
    if (!gd || !el || !dc_edev || !dc_pdev)
        return (false);
    if (el->cdesc()->masterCell() != dc_edev->cdesc()->masterCell())
        return (false);
    if (!dc_pdev->desc()->is_mos())
        return (false);

    int ng1 = -1;
    int ns1 = -1;
    int nd1 = -1;
    for (unsigned int i = 0; i < dc_nodes_sz; i++) {
        switch (*Tstring(dc_conts[i]->cont_name())) {
        case 'g':
        case 'G':
            ng1 = dc_nodes[i]->enode();
            break;
        case 'd':
        case 'D':
            nd1 = dc_nodes[i]->enode();
            break;
        case 's':
        case 'S':
            ns1 = dc_nodes[i]->enode();
            break;
        }
    }

    // Avoid malloc with a fixed-size array, sane mosfets have four
    // nodes but allow some insanity.
    if (dc_nodes_sz > 6)
        return (false);
    CDp_cnode *nodes[6];
    int ng2 = -1;
    int ns2 = -1;
    int nd2 = -1;
    CDp_cnode *pc0 = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
    int vix = el->cdesc_index();
    CDp_range *pr = 0;
    if (vix > 0) {
        pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
        if (!pr)
            return (false);
    }
    for ( ; pc0; pc0 = pc0->next()) {
        CDp_cnode *pc;
        if (vix > 0)
            pc = pr->node(0, vix, pc0->index());
        else
            pc = pc0;
        if (!pc)
            return (false);
        switch (*Tstring(dc_conts[pc->index()]->cont_name())) {
        case 'g':
        case 'G':
            ng2 = pc->enode();
            break;
        case 'd':
        case 'D':
            nd2 = pc->enode();
            break;
        case 's':
        case 'S':
            ns2 = pc->enode();
            break;
        }
        nodes[pc->index()] = pc;
    }

    // Gate groups the same, and associated?
    if (ng1 != ng2 || gd->group_of_node(ng1) < 0)
        return (false);

    // One common permutable group connection which is not global?
    int n1, n2, nc;
    if (nd1 == nd2) {
        n1 = ns1;
        n2 = ns2;
        nc = nd1;
    }
    else if (nd1 == ns2) {
        n1 = ns1;
        n2 = nd2;
        nc = nd1;
    }
    else if (ns1 == nd2) {
        n1 = nd1;
        n2 = ns2;
        nc = ns1;
    }
    else if (ns1 == ns2) {
        n1 = nd1;
        n2 = nd2;
        nc = ns1;
    }
    else
        return (false);

    // Group associated and not global?
    int gc = gd->group_of_node(nc);
    sGroup *g = gd->group_for(gc);
    if (!g || g->global())
        return (false);

    // Test separately for parallel devices.
    if (n1 == n2)
        return (false);

    // The other permutable contact should connect to exactly one
    // permutable contact of a similar device, so the termlists should
    // have exactly two entries.

    CDcont *t1 = gd->conts_of_node(n1);
    if (!t1 || !t1->next() || t1->next()->next())
        return (false);
    if (!t1->term()->instance() || !t1->next()->term()->instance())
        return (false);

    CDcterm *trm1;
    CDp_cnode *pc1 = t1->term()->node_prpty();
    CDp_cnode *pc2 = t1->next()->term()->node_prpty();
    if (pc1->index() >= dc_nodes_sz || pc2->index() >= dc_nodes_sz)
        return (false);
    if (pc1 == dc_nodes[pc1->index()])
        trm1 = t1->next()->term();
    else if (pc2 == dc_nodes[pc2->index()])
        trm1 = t1->term();
    else
        return (false);

    // Check that device is similar, and contact is permutable.
    if (trm1->instance()->masterCell() != dc_edev->cdesc()->masterCell())
        return (false);
    if (trm1->node_prpty()->index() != (unsigned int)dc_pix1 &&
            trm1->node_prpty()->index() != (unsigned int)dc_pix2)
        return (false);

    CDcont *t2 = gd->conts_of_node(n2);
    if (!t2 || !t2->next() || t2->next()->next())
        return (false);
    if (!t2->term()->instance() || !t2->next()->term()->instance())
        return (false);

    CDcterm *trm2;
    pc1 = t2->term()->node_prpty();
    pc2 = t2->next()->term()->node_prpty();
    if (pc1->index() >= dc_nodes_sz || pc2->index() >= dc_nodes_sz)
        return (false);
    if (pc1 == nodes[pc1->index()])
        trm2 = t2->next()->term();
    else if (pc2 == nodes[pc2->index()])
        trm2 = t2->term();
    else
        return (false);

    // Check that device is similar, and contact is permutable.
    if (trm2->instance()->masterCell() != dc_edev->cdesc()->masterCell())
        return (false);
    if (trm2->node_prpty()->index() != (unsigned int)dc_pix1 &&
            trm2->node_prpty()->index() != (unsigned int)dc_pix2)
        return (false);

    // Find the gate nodes of the connected devices.  If the nodes are
    // the same, we're golden.

    CDc *cd1 = trm1->instance();
    int vix1 = trm1->inst_index();
    ng1 = gate_node(cd1, vix1, dc_gix);
    if (ng1 < 0)
        return (false);

    CDc *cd2 = trm2->instance();
    int vix2 = trm2->inst_index();
    ng2 = gate_node(cd2, vix2, dc_gix);
    if (ng2 < 0)
        return (false);

    return (ng1 == ng2);
}

