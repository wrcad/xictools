
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
#include "ext_duality.h"
#include "ext_ep_comp.h"
#include "ext_nets.h"
#include "ext_errlog.h"
#include "sced.h"
#include "sced_nodemap.h"
#include "sced_param.h"
#include "cd_netname.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "promptline.h"
#include "select.h"
#include "miscutil/timer.h"
#include <algorithm>

#define TIME_DBG
#ifdef TIME_DBG
//#define TIME_DBG_XTRA
#include "miscutil/timedbg.h"
#endif


/*========================================================================*
 *
 *  Functions for establishing physical/electrical duality
 *
 *========================================================================*/

// Default limits apply when associating.
int cGroupDesc::gd_loop_max = EXT_DEF_LVS_LOOP_MAX;
int cGroupDesc::gd_iter_max = EXT_DEF_LVS_ITER_MAX;

// destination for error/progress messages
#define D_LOGNAME "associate.log"

// When associating devices and subcircuits, we calculate the ratio of
// the number of already associated contacts to the number of
// contacts, and prioritize the highest ratio (i.e., attempt to
// associate them first).  We will use quintiles, top quintile (80% -
// 100%) first.
//
#define NUM_LEVELS 5
#define USE_LEVELS


// This is the main function for establishing duality to the electrical
// part of the database.  This processes the entire hierarchy.
//
bool
cExt::associate(CDs *sdesc)
{
    if (!sdesc)
        return (true);
    if (sdesc->isElectrical())
        return (false);
    if (sdesc->isAssociated())
        return (true);

    if (!extract(sdesc))
        return (false);
    cGroupDesc *gd = sdesc->groups();
    if (!gd)
        return (true);
    CDs *esdesc = CDcdb()->findCell(sdesc->cellname(), Electrical);
    if (!esdesc)
        return (true);

    // We need theschematic connected at all levels before
    // association.
    SCD()->connectAll(false, esdesc);

#ifdef TIME_DBG
    Tdbg()->start_timing("duality_total");
#endif
    PL()->ShowPrompt("Associating ...");

    // Electrical connectivity ignores shorted NOPHYS devices.
    SCD()->setIncludeNoPhys(false);

    dspPkgIf()->SetWorking(true);

    bool ttmp = DSP()->ShowTerminals();
    if (ttmp)
        DSP()->ShowTerminals(ERASE);

    gd->set_top_level(true);

    // For evaluating electrical device parameters.
    cParamCx *pcx = new cParamCx(esdesc);
    setParamCx(pcx);

    ExtErrLog.start_logging(ExtLogAssoc, sdesc->cellname());
    SymTab done_tab(false, false);
    XIrt ret = gd->setup_duality_first_pass(&done_tab);
    if (ret == XIok)
        ret = gd->setup_duality();
    ExtErrLog.add_log(ExtLogAssoc, "Associating %s complete, %s.",
        Tstring(sdesc->cellname()),
        ret == XIok ? "no errors" : "error(s) encountered");
    ExtErrLog.end_logging();

    setParamCx(0);
    delete pcx;

    gd->set_top_level(false);

    if (ret == XIok)
        PL()->ShowPromptV("Association complete in %s.",
            Tstring(sdesc->cellname()));
    else
        PL()->ShowPrompt("Association aborted.");

    // Terminals may have moved!
    if (ttmp)
        DSP()->ShowTerminals(DISPLAY);

    if (isUpdateNetLabels()) {
        // Unless inhibited, check and update net name labels throughout
        // the hierarchy.
        updateNetLabels();
    }
    if (isUseMeasurePrpty()) {
        // Unless inhibited, update the measure results to a property
        // throughout the hierarchy.
        saveMeasuresInProp();
    }

#ifdef TIME_DBG
    Tdbg()->accum_timing("duality_total");
    Tdbg()->print_accum("first_pass");
#ifdef TIME_DBG_XTRA
    Tdbg()->print_accum("first_pass_setup1");
    Tdbg()->print_accum("first_pass_setup2");
    Tdbg()->print_accum("first_pass_setup3");
    Tdbg()->print_accum("first_pass_setup4");
    Tdbg()->print_accum("first_pass_setup5");
    Tdbg()->print_accum("first_pass_setup6");
#else
    Tdbg()->print_accum("first_pass_setup");
#endif
    Tdbg()->print_accum("first_pass_solve");
    Tdbg()->print_accum("first_pass_misc");
    Tdbg()->print_accum("second_pass");
    Tdbg()->print_accum("measure_devices");
    Tdbg()->print_accum("duality_total");
#endif
    PopUpSelections(0, MODE_UPD);

    // We're done with this.
    updateReferenceTable(0);

    dspPkgIf()->SetWorking(false);
    return (ret == XIok);
}


// Export to find group for node.
//
int
cExt::groupOfNode(CDs *sdesc, int node)
{
    if (sdesc && node >= 0) {
        cGroupDesc *gd = 0;
        if (sdesc->isElectrical()) {
            CDs *sd = CDcdb()->findCell(sdesc->cellname(), Physical);
            if (sd)
                gd = sd->groups();
        }
        else
            gd = sdesc->groups();
        if (gd)
            return (gd->group_of_node(node));
    }
    return (-1);
}


// Export to find node for group.
//
int
cExt::nodeOfGroup(CDs *sdesc, int grp)
{
    if (sdesc && grp >= 0) {
        cGroupDesc *gd = 0;
        if (sdesc->isElectrical()) {
            CDs *sd = CDcdb()->findCell(sdesc->cellname(), Physical);
            if (sd)
                gd = sd->groups();
        }
        else
            gd = sdesc->groups();
        if (gd)
            return (gd->node_of_group(grp));
    }
    return (-1);
}


void
cExt::clearFormalTerms(CDs *sdesc)
{
    cGroupDesc *gd = 0;
    if (sdesc->isElectrical()) {
        CDs *sd = CDcdb()->findCell(sdesc->cellname(), Physical);
        if (sd)
            gd = sd->groups();
    }
    else
        gd = sdesc->groups();
    if (gd)
        gd->clear_formal_terms();
}
// End of cExt functions.


XIrt
cGroupDesc::setup_duality_first_pass(SymTab *done_tab, int dcnt)
{
    if (dcnt >= CDMAXCALLDEPTH)
        return (XIbad);

    if (SymTab::get(done_tab, (unsigned long)this) != ST_NIL)
        return (XIok);
    if (!gd_devices && !gd_subckts) {
        // Nothing any good here, silently skip it.

        clear_duality();
        done_tab->add((unsigned long)this, 0, false);
        return (XIok);
    }

    XIrt ret = XIok;
    {
        // Have to work from the bottom up.
        CDm_gen mgen(gd_celldesc, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
            cGroupDesc *gd = md->celldesc()->groups();
            if (gd) {
                ret = gd->setup_duality_first_pass(done_tab, dcnt + 1);
                if (ret != XIok)
                    break;
            }
        }
    }
    if (ret == XIok && !gd_celldesc->isAssociated()) {

#ifdef TIME_DBG
        Tdbg()->start_timing("first_pass");
#endif
        for (sSubcList *sc = gd_subckts; sc; sc = sc->next()) {
            for (sSubcInst *su = sc->subs(); su; su = su->next())
                su->pre_associate();
        }

        set_first_pass(true);
        ret = set_duality_first_pass();
        set_first_pass(false);

        if (ret != XIok)
            clear_duality();

#ifdef TIME_DBG
        Tdbg()->accum_timing("first_pass");
#endif
    }

    done_tab->add((unsigned long)this, 0, false);
    return (ret);
}


// This is called recursively to associate devices and subcircuits.
//
XIrt
cGroupDesc::setup_duality(int dcnt)
{
    if (dcnt >= CDMAXCALLDEPTH)
        return (XIbad);

    if (!gd_devices && !gd_subckts) {
        // Nothing any good here, silently skip it.

        clear_duality();
        gd_celldesc->setAssociated(true);
        return (XIok);
    }

    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    // A null or empty esdesc is ok here, the physical contents needs
    // to be recursed through.

    if (!top_level() && EX()->paramCx())
        EX()->paramCx()->push(esdesc, 0);

    XIrt ret = XIok;
    {
        // Have to work from the bottom up.
        CDm_gen mgen(gd_celldesc, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
            cGroupDesc *gd = md->celldesc()->groups();
            if (gd) {
                ret = gd->setup_duality(dcnt + 1);
                if (ret != XIok)
                    break;
            }
        }
    }
    if (ret == XIok && !gd_celldesc->isAssociated()) {

#ifdef TIME_DBG
        Tdbg()->start_timing("second_pass");
#endif
        ret = set_duality();

        if (ret != XIok)
            clear_duality();
        else {
            for (sSubcList *sc = gd_subckts; sc; sc = sc->next()) {
                fixup_duality(sc);
                if (EX()->isSubcPermutationFix() && top_level())
                    subcircuit_permutation_fix(sc);
            }

            // Name the physical nets from the named electrical nets.
            init_net_names();

            for (sSubcList *sc = gd_subckts; sc; sc = sc->next()) {
                for (sSubcInst *su = sc->subs(); su; su = su->next())
                    su->post_associate();
            }

            // Make sure that the Extracted flag is also set, it can get
            // turned off by, e.g., unsetConnected().
            gd_celldesc->setExtracted(true);
            gd_celldesc->setAssociated(true);
        }
#ifdef TIME_DBG
        Tdbg()->accum_timing("second_pass");
#endif
    }
    if (!top_level() && esdesc && EX()->paramCx())
        EX()->paramCx()->pop();

    return (ret);
}


// In cells with metal areas that are connected to cell terminals
// only, the terminals won't be associated.  If there is more than one
// such terminal, there is no way to associate, as we don't have
// enough information.  After the hierarchy is associated, we look
// top-down into the subcells, and use instance connections to infer
// the unresolved terminal associations.
//

void
cGroupDesc::fixup_duality(const sSubcList *sl)
{
    CDs *psd = sl->subs()->cdesc()->masterCell();
    CDcbin cbin(psd);
    if (!cbin.elec() || !cbin.phys())
        return;
    cGroupDesc *mgd = cbin.phys()->groups();
    if (!mgd)
        return;


    int nfixes = 0;
    for (CDp_snode *ps = (CDp_snode*)cbin.elec()->prpty(P_NODE); ps;
            ps = ps->next()) {
        CDsterm *term = ps->cell_terminal();
        if (!term || term->group() >= 0)
            continue;

        ExtErrLog.add_log(ExtLogAssoc, "fixup: cell %s terminal %s",
            Tstring(cbin.cellname()), term->name());

        // Find node of unplaced terminal.
        int node = ps->enode();
        unsigned int index = ps->index();
        bool fixed = false;

        // Look through the instances of the cell in the parent.
        for (sEinstList *el = sl->esubs(); el; el = el->next()) {
            if (!el->dual_subc())
                continue;

            CDp_cnode *pn = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
            if (el->cdesc_index() > 0) {
                CDp_range *pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
                if (!pr)
                    continue;
                pn = pr->node(0, el->cdesc_index(), index);
            }
            else {
                for ( ; pn; pn = pn->next()) {
                    if (pn->index() == index)
                        break;
                }
            }
            if (!pn)
                continue;

            int parent_node = pn->enode();
            int parent_group = group_of_node(parent_node);
            if (parent_node < 0 || parent_group < 0)
                continue;

            // Find instance group, this should be correct group for node.
            sSubcInst *s = el->dual_subc();
            for (sSubcContactInst *ci = s->contacts(); ci; ci = ci->next()) {
                if (ci->parent_group() < 0 || ci->subc_group() < 0)
                    continue;
                if (gd_groups[ci->parent_group()].node() ==
                        parent_node || ci->parent_group() == parent_group) {

                    // Not sure why this happens, fix it.
                    if (parent_group < 0)
                        gd_etlist->set_group(parent_node, ci->parent_group());

                    int grp = ci->subc_group();
                    mgd->gd_groups[grp].set_node(node);
                    mgd->gd_etlist->set_group(node, grp);
                    ExtErrLog.add_log(ExtLogAssoc,
                        "Associating node %d to group %d, (in fixup).",
                        node, grp);
                    fixed = true;
                    break;
                }
            }
            if (fixed) {
                nfixes++;
                break;
            }
        }
    }
    if (nfixes)
        mgd->reposition_terminals();
}


// Return a list of permutable groups identified via topological
// examination.  These are formal terminal groups only.
//
sPermGrpList *
cGroupDesc::check_permutes() const
{
    CDpin *pins = list_cell_terms();
    int nterms = 0;
    for (CDpin *p1 = pins; p1; p1 = p1->next())
        nterms++;
    if (nterms < 2) {
        CDpin::destroy(pins);
        return (0);
    }
    CDsterm **ary = new CDsterm*[nterms];
    nterms = 0;
    for (CDpin *p1 = pins; p1; p1 = p1->next())
        ary[nterms++] = p1->term();
    CDpin::destroy(pins);

    sPermGrpList *pgl = 0;
    int find_cnt = 0;
    PGtype type = PGtopo;
    int groups[4];

    for (int i = 0; i < nterms; i++) {
        if (!ary[i])
            continue;

        // Check for permutable MOS gate inputs.
        sDevInst *nmos1 = 0, *pmos1 = 0;
        int dcnt1;
        if (mos_np_input(ary[i], &nmos1, &pmos1, &dcnt1)) {
            int group1 = ary[i]->group();
            for (int j = i+1; j < nterms; j++) {
                if (!ary[j])
                    continue;
                sDevInst *nmos2 = 0, *pmos2 = 0;
                int dcnt2;
                if (!mos_np_input(ary[j], &nmos2, &pmos2, &dcnt2))
                    continue;
                if ((nmos1 != 0) != (nmos2 != 0))
                    continue;
                if ((pmos1 != 0) != (pmos2 != 0))
                    continue;
                if (dcnt1 != dcnt2)
                    continue;
                if (!pmos1)
                    type = PGnor;
                else if (!nmos1)
                    type = PGnand;
                int group2 = ary[j]->group();

                if (type != PGnor) {
                    if (nand_match(group1, group2, pmos1, pmos2, dcnt1)) {
                        if (find_cnt == 0) {
                            groups[0] = group1;
                            groups[1] = group2;
                            ary[j] = 0;
                            find_cnt = 2;
                            type = PGnand;
                        }
                        else if (type == PGnand) {
                            if (find_cnt == 4)
                                break;
                            groups[find_cnt++] = group2;
                            ary[j] = 0;
                        }
                        continue;
                    }
                }
                if (type != PGnand) {
                    if (nor_match(group1, group2, nmos1, nmos2, dcnt1)) {
                        if (find_cnt == 0) {
                            groups[0] = group1;
                            groups[1] = group2;
                            ary[j] = 0;
                            find_cnt = 2;
                            type = PGnor;
                        }
                        else if (type == PGnor) {
                            if (find_cnt == 4)
                                break;
                            groups[find_cnt++] = group2;
                            ary[j] = 0;
                        }
                        continue;
                    }
                }
            }
            if (find_cnt > 1)
                pgl = new sPermGrpList(groups, find_cnt, type, pgl);
            find_cnt = 0;
            type = PGtopo;
            continue;
        }

        // Check for permutable inputs, by topology.

        int node1 = ary[i]->node_prpty()->enode();
        int group1 = ary[i]->group();
        if (node1 >= 0 && group1 >= 0) {
            for (int j = i+1; j < nterms; j++) {
                if (!ary[j])
                    continue;
                int node2 = ary[j]->node_prpty()->enode();
                if (gd_etlist->is_permutable(node1, node2)) {
                    int group2 = ary[j]->group();
                    if (group2 >= 0) {
                        if (find_cnt == 0) {
                            groups[0] = group1;
                            groups[1] = group2;
                            ary[j] = 0;
                            find_cnt = 2;
                        }
                        else {
                            if (find_cnt == 4)
                                break;
                            groups[find_cnt++] = group2;
                            ary[j] = 0;
                        }
                    }
                }
            }
            if (find_cnt > 1)
                pgl = new sPermGrpList(groups, find_cnt, type, pgl);
            find_cnt = 0;
        }
    }
    delete [] ary;
    return (pgl);
}


// This is called when there are no association errors except for
// terminal referencing, which indicates that node association differs
// from the schematic, though it is topologically equivalent.  This is
// expected, as we use arbitrary symmetry breaking to force
// association when there is insufficient information.  When
// instantiated, correct connection to the parent will likely require
// a call to this function, which will re-associate the connections.
//
// This descends recursively through the hierarchy, applying the fix. 
// It should be called from the top level, after association of the
// top-level cell.
//
void
cGroupDesc::subcircuit_permutation_fix(const sSubcList *sl)
{
    CDs *psd = sl->subs()->cdesc()->masterCell();
    CDcbin cbin(psd);
    if (!cbin.elec() || !cbin.phys())
        return;
    cGroupDesc *mgd = cbin.phys()->groups();
    if (!mgd)
        return;

    sSubcDesc *scd = EX()->findSubcircuit(cbin.phys());
    bool has_pgs = (scd && scd->num_groups() > 0);

    int nfixes = 0;
    if (gd_discreps == 0 && mgd->gd_discreps == 0) {
        // This applies only when parent and subcircuit are both
        // "clean" except for terminal referencing errors.

        // We'll save our corrections in this table.  The key is the
        // group number, data item is the "good" node number that
        // differs from the present.
        //
        SymTab tab(false, false);

        for (sSubcInst *si = sl->subs(); si; si = si->next()) {
            if (!si->dual())
                return;  // "can't happen"
            if (!si->contacts() || !si->contacts()->next())
                return;

            CDc *cdesc = si->dual()->cdesc();
            int vecix = si->dual()->cdesc_index();
            for (sSubcContactInst *ci = si->contacts(); ci; ci = ci->next()) {
                if (ci->parent_group() < 0 || ci->subc_group() < 0)
                    continue;

                sGroup &pgrp = gd_groups[ci->parent_group()];
                if (pgrp.global())
                    continue;
                int node = pgrp.node();
                if (node < 0)
                    continue;

                sGroup &sgrp = mgd->gd_groups[ci->subc_group()];
                if (sgrp.node() < 0)
                    continue;

                // Don't permute if labeled.
                if (sgrp.netname() &&
                        sgrp.netname_origin() == sGroup::NameFromLabel)
                    continue;

                if (has_pgs) {
                    // The subcell has permutation groups of terminals
                    // recognized as equivalent gate inputs.  These
                    // have already been accounted for and should be
                    // skipped here.

                    bool found = false;
                    int numg = scd->num_groups();
                    for (int i = 0; i < numg && !found; i++) {
                        int gs = scd->group_size(i);
                        int *a = scd->group_ary(i);
                        for (int j = 0; j < gs; j++) {
                            if (a[j] == ci->subc_group()) {
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found)
                        continue;
                }

                // The subcircuit association is assumend valid here.
                int esg = -1;
                CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
                CDp_range *pr = 0;
                if (vecix > 0) {
                    pr = (CDp_range*)cdesc->prpty(P_RANGE);
                    if (!pr)
                        continue;
                }
                for ( ; pc; pc = pc->next()) {
                    CDp_cnode *pc1;
                    if (vecix > 0) {
                        pc1 = pr->node(0, vecix, pc->index());
                        if (!pc1)
                            continue;
                    }
                    else
                        pc1 = pc;
                    if (pc1->enode() == node) {
                        CDp_snode *ps = (CDp_snode*)cbin.elec()->prpty(P_NODE);
                        for ( ; ps; ps = ps->next()) {
                            if (ps->index() == pc1->index())
                                break;
                        }
                        if (ps) {
                            int g = mgd->group_of_node(ps->enode());
                            if (g == ci->subc_group()) {
                                esg = -1;
                                break;
                            }
                            else {
                                if (esg < 0)
                                    esg = g;
                                else {
                                    esg = -1;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (esg < 0)
                    continue;

                unsigned long old_sg = ci->subc_group();
                unsigned long new_nd = mgd->gd_groups[esg].node();

                // Check to be sure that each instance connection is
                // consistent.  Put a message in the log.  LVS will
                // fail.

                unsigned long oldn = (unsigned long)SymTab::get(&tab, old_sg);

                if (oldn == (unsigned long)ST_NIL)
                    tab.add(old_sg, (void*)new_nd, false);
                else if (oldn != new_nd) {
                    char *iname = si->instance_name();
                    ExtErrLog.add_log(ExtLogAssoc,
                        "Permuting nodes in %s, instance %s has "
                        "inconsistency,\ngroup %ld, node %ld or %ld.",
                        Tstring(gd_celldesc->cellname()),
                        iname, old_sg, new_nd, oldn);
                    delete [] iname;
                }
            }
        }

        // Use the table to set the hint_node field of the group
        // descriptor.

        SymTabGen gen(&tab);
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            int grp = (long)ent->stTag;
            int nd = (long)ent->stData;
            if (ExtErrLog.log_associating() && ExtErrLog.verbose()) {
                int ndcur = mgd->gd_groups[grp].node();
                const char *nncur = SCD()->nodeName(cbin.elec(), ndcur);
                const char *nnhint = SCD()->nodeName(cbin.elec(), nd);
                ExtErrLog.add_log(ExtLogAssocV,
                    "In %s, hinting node %d (%s) to group %d, "
                    "was node %d (%s).",
                    Tstring(mgd->gd_celldesc->cellname()),
                    nd, nnhint, grp, ndcur, nncur);
            }
            mgd->gd_groups[grp].set_hint_node(nd);
            nfixes++;
        }

        ExtErrLog.add_log(ExtLogAssoc,
            "\nPermutation check/fix in %s, %d fixes.",
            Tstring(mgd->gd_celldesc->cellname()), nfixes);

        mgd->subcircuit_permutation_fix_rc(nfixes);
    }
}


// Remove the duality establishment from the cGroupDesc.
//
void
cGroupDesc::clear_duality()
{
    bool ttmp = DSP()->ShowTerminals();
    if (ttmp)
        DSP()->ShowTerminals(ERASE);

    // Delete electrical devices from the device list.
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        sEinstList::destroy(dv->edevs());
        dv->set_edevs(0);
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next())
                di->set_dual(0);
        }
    }
    sEinstList::destroy(gd_extra_devs);
    gd_extra_devs = 0;

    // Delete electrical subcells in the subckts list.
    for (sSubcList *su = gd_subckts; su; su = su->next()) {
        sEinstList::destroy(su->esubs());
        su->set_esubs(0);
        for (sSubcInst *s = su->subs(); s; s = s->next())
            s->clear_duality();
    }
    sEinstList::destroy(gd_extra_subs);
    gd_extra_subs = 0;

    for (int i = 1; i < gd_asize; i++) {
        sGroup &g = gd_groups[i];
        g.set_node(-1);
        g.set_split_group(-1);
        g.clear_flags();

        // Keep only netnames obtained from labels and terminals.
        if ((int)g.netname_origin() < sGroup::NameFromTerm)
            g.set_netname(0, sGroup::NameFromLabel);

        // Keep only TE_FIXED terminals.
        CDpin *pp = 0, *pn;
        for (CDpin *p = g.termlist(); p; p = pn) {
            pn = p->next();
            if (!p->term()->is_fixed()) {
                p->term()->set_ref(0);
                if (pp)
                    pp->set_next(pn);
                else
                    g.set_termlist(pn);
                delete p;
                continue;
            }
            pp = p;
        }
    }

    delete gd_etlist;
    gd_etlist = 0;
    delete gd_global_nametab;
    gd_global_nametab = 0;
    sSymBrk::destroy(gd_sym_list);
    gd_sym_list = 0;

    set_skip_permutes(false);
    set_grp_break(false);

    gd_celldesc->reflectBadAssoc();
    if (ttmp)
        DSP()->ShowTerminals(DISPLAY);
}


// Given an electrical device cdesc, return the physical device
// instance.  The vec_ix is the index in the case that cdesc is
// vectored.
//
sDevInst *
cGroupDesc::find_dual_dev(const CDc *cdesc, int vec_ix)
{
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (di->dual()) {
                    if (di->dual()->cdesc()->master() != cdesc->master())
                        break;
                    if (di->dual()->cdesc() == cdesc &&
                            di->dual()->cdesc_index() == vec_ix)
                        return (di);
                }
            }
        }
    }
    return (0);
}


// Given an electrical subcircuit cdesc, return the physical
// subcircuit instance.  The vec_ix is the index in the case that
// cdesc is vectored.
//
sSubcInst *
cGroupDesc::find_dual_subc(const CDc *cdesc, int vec_ix)
{
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        // Note: all cellnames are in string table.
        if (sl->subs()->cdesc()->cellname() != cdesc->cellname())
            continue;
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (s->dual() && s->dual()->cdesc() == cdesc &&
                    s->dual()->cdesc_index() == vec_ix)
                return (s);
        }
        break;
    }
    return (0);
}


// Assign a group to the terminal.  Move the terminal if necessary to
// place it in the group.  True is returned if the terminal is
// associated with a group.
//
// This applies only to formal terminals.  Instance terminals are
// placed by transforming the master terminal locations.  Instance
// terminals have no reference object, and CDterm::group always
// returns -1.
//
bool
cGroupDesc::bind_term_to_group(CDsterm *term, int group)
{
    if (term->instance())
        return (false);
    if (group < 0 || group >= gd_asize)
        return (false);

    int ogrp = term->group();
    if (ogrp == group)
        // already done
        return (true);

    if (term->is_fixed()) {
        // The terminal can't be moved.  If it doesn't connect to the
        // group as it is, we're bad.

        if (!bind_term_group_at_location(term))
            return (false);
        ogrp = term->group();
        if (ogrp < 0) {
            ExtErrLog.add_log(ExtLogAssoc,
                "Couldn't link fixed terminal %s.", term->name());
            return (false);
        }
        if (ogrp == group)
            return (true);

        // Bad.  Fixed terminal appears to be on wrong group.
        ExtErrLog.add_log(ExtLogAssoc,
            "Terminal %s has fixed location on group %d, but I "
            "think it\nbelongs on group %d.", term->name(), ogrp, group);
        return (false);
    }

    if (!gd_groups[group].net()) {
        // virtual terminal
        return (bind_term_to_vgroup(term, group));
    }

    // If the terminal is already bound to a group, unbind it.
    if (ogrp >= 0) {
        term->set_ref(0);
        CDpin *pp = 0, *pn;
        for (CDpin *p = gd_groups[ogrp].termlist(); p; p = pn) {
            pn = p->next();
            if (p->term() == term) {
                if (pp)
                    pp->set_next(pn);
                else
                    gd_groups[ogrp].set_termlist(pn);
                delete p;
                break;
            }
            pp = p;
        }
    }

    // Formal terminals are connected preferentially to ROUTING
    // layers.  Move formal terminals only if necessary.  A formal
    // terminal can use a CONDUCTOR layer only if the net has no
    // ROUTING layer objects.

    BBox BB;
    bool firstone = true;
    if (!term->layer() || term->layer()->isRouting()) {
        for (CDol *o = gd_groups[group].net()->objlist(); o; o = o->next) {
            if (!o->odesc->ldesc()->isRouting())
                continue;
            if (!term->layer() || term->layer() == o->odesc->ldesc()) {
                if (o->odesc->intersect(term->lx(), term->ly(), true)) {
                    // location is ok
                    term->set_ref(o->odesc);
                    bind_term(term, group);
                    return (true);
                }
                if (firstone) {
                    BB = o->odesc->oBB();
                    firstone = false;
                }
                else
                    BB.add(&o->odesc->oBB());
            }
        }
    }

    bool no_layer = false;
    if (firstone && term->layer()) {
        // Try again, ignoring layer hint.
        for (CDol *o = gd_groups[group].net()->objlist(); o; o = o->next) {
            if (!o->odesc->ldesc()->isRouting())
                continue;
            if (o->odesc->intersect(term->lx(), term->ly(), true)) {
                // location is ok
                term->set_ref(o->odesc);
                bind_term(term, group);
                return (true);
            }
            if (firstone) {
                BB = o->odesc->oBB();
                firstone = false;
            }
            else
                BB.add(&o->odesc->oBB());
            no_layer = true;
        }
    }

    bool allow_cdtr = false;
    if (firstone) {
        // Uh-oh, no routing conductor.  Use a conductor in this case,
        // but issue warning.
        for (CDol *o = gd_groups[group].net()->objlist(); o; o = o->next) {
            if (!o->odesc->ldesc()->isConductor())
                continue;
            if (!term->layer() || term->layer() == o->odesc->ldesc()) {
                if (o->odesc->intersect(term->lx(), term->ly(), true)) {
                    // location is ok
                    term->set_ref(o->odesc);
                    bind_term(term, group);
                    warn_conductor(group, Tstring(term->name()));
                    return (true);
                }
                if (firstone) {
                    BB = o->odesc->oBB();
                    firstone = false;
                }
                else
                    BB.add(&o->odesc->oBB());
                allow_cdtr = true;
            }
        }
    }
    if (firstone && term->layer()) {
        // Try again, ignoring layer hint.
        for (CDol *o = gd_groups[group].net()->objlist(); o; o = o->next) {
            if (!o->odesc->ldesc()->isConductor())
                continue;
            if (o->odesc->intersect(term->lx(), term->ly(), true)) {
                // location is ok
                term->set_ref(o->odesc);
                bind_term(term, group);
                warn_conductor(group, Tstring(term->name()));
                return (true);
            }
            if (firstone) {
                BB = o->odesc->oBB();
                firstone = false;
            }
            else
                BB.add(&o->odesc->oBB());
            allow_cdtr = true;
            no_layer = true;
        }
    }

    BB.bloat(-10);
    const BBox *sBB = gd_celldesc->BB();

    int dl = BB.left - sBB->left;
    int db = BB.bottom - sBB->bottom;
    int dr = sBB->right - BB.right;
    int dt = sBB->top - BB.top;
    int d = mmMin(mmMin(dl, dr), mmMin(dt, db));
    if (d == dl) {
        BB.right = BB.left;
        BB.left -= 10;
    }
    else if (d == dt) {
        BB.bottom = BB.top;
        BB.top += 10;
    }
    else if (d == dr) {
        BB.left = BB.right;
        BB.right += 10;
    }
    else {
        BB.top = BB.bottom;
        BB.bottom -= 10;
    }
    if (gd_groups[group].net()) {
        for (CDol *o = gd_groups[group].net()->objlist(); o; o = o->next) {
            if (!allow_cdtr && !o->odesc->ldesc()->isRouting())
                continue;
            if (!no_layer && term->layer() &&
                    term->layer() != o->odesc->ldesc())
                continue;

            if (o->odesc->intersect(&BB, false)) {
                CDo *oset = o->odesc;
                bool setit = false;
                if (oset->type() == CDBOX) {
                    if (oset->oBB().left > BB.left)
                        BB.left = oset->oBB().left;
                    if (oset->oBB().bottom > BB.bottom)
                        BB.bottom = oset->oBB().bottom;
                    if (oset->oBB().right < BB.right)
                        BB.right = oset->oBB().right;
                    if (oset->oBB().top < BB.top)
                        BB.top = oset->oBB().top;
                    term->set_loc((BB.left + BB.right)/2,
                        (BB.bottom + BB.top)/2);
                    setit = true;
                }
                else if (oset->type() == CDPOLYGON || oset->type() == CDWIRE) {
                    Zoid Z(&BB);
                    Zlist *z0 = oset->toZlist();
                    for (Zlist *z = z0; z; z = z->next) {
                        Zlist *zx = Z.clip_to(&z->Z);
                        if (zx) {
                            term->set_loc((zx->Z.xll + zx->Z.xul +
                                zx->Z.xlr + zx->Z.xur)/4,
                                (zx->Z.yl + zx->Z.yu)/2);
                            Zlist::destroy(zx);
                            setit = true;
                            break;
                        }
                    }
                    Zlist::destroy(z0);
                }
                if (setit) {
                    term->set_ref(oset);
                    bind_term(term, group);
                    if (!oset->ldesc()->isRouting())
                        warn_conductor(group, Tstring(term->name()));
                    return (true);
                }
            }
        }
    }
    ExtErrLog.add_log(ExtLogAssoc, "Couldn't place terminal %s.",
        term->name());
    return (false);
}


// Remove the formal terminals from the groups.  This is necessary if
// the terminals are deleted (see clear_cell() in spiceprp.cc).
//
void
cGroupDesc::clear_formal_terms()
{
    for (int i = 0; i < gd_asize; i++) {
        CDpin::destroy(gd_groups[i].termlist());
        gd_groups[i].set_termlist(0);
    }
}


void
cGroupDesc::set_association(int grp, int node)
{
    sGroup *g = group_for(grp);
    if (g) {
        const char *msg_g = gd_sym_list ?
            "Assigning node %d to group %d (symmetry trial)." :
            "Assigning node %d to group %d.";

        g->set_node(node);
        gd_etlist->set_group(node, grp);
        ExtErrLog.add_log(ExtLogAssoc, msg_g, node, grp);
        if (gd_sym_list)
            gd_sym_list->new_grp_assoc(grp, node);
    }
}


void
cGroupDesc::select_unassoc_groups()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    for (int i = 0; i < gd_asize; i++) {
        if (gd_groups[i].node() < 0) {
            if (gd_groups[i].net()) {
                for (CDol *o = gd_groups[i].net()->objlist(); o; o = o->next) {
                    o->odesc->set_state(CDVanilla);
                    Selections.insertObject(cursdp, o->odesc);
                }
            }
        }
    }
}


void
cGroupDesc::select_unassoc_nodes()
{
}


void
cGroupDesc::select_unassoc_pdevs()
{
    sDevInstList *d0 = 0;
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (!di->dual())
                    d0 = new sDevInstList(di, d0);
            }
        }
    }
    if (d0) {
        EX()->queueDevices(d0);
        sDevInstList::destroy(d0);
    }
}


void
cGroupDesc::select_unassoc_edevs()
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde || cursde->isSymbolic())
        return;
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sEinstList *c = dv->edevs(); c; c = c->next()) {
            if (!c->dual_dev()) {
                CDc *cd = c->cdesc();
                const char *instname = cd->getElecInstBaseName();
                // ignore wire caps
                if (instname && !lstring::prefix(WIRECAP_PREFIX, instname)) {
                    cd->set_state(CDVanilla);
                    Selections.insertObject(cursde, cd);
                }
            }
        }
    }
    if (gd_extra_devs) {
        for (sEinstList *c = gd_extra_devs; c; c = c->next()) {
            CDc *cd = c->cdesc();
            const char *instname = cd->getElecInstBaseName();
            // ignore wire caps
            if (instname && !lstring::prefix(WIRECAP_PREFIX, instname)) {
                cd->set_state(CDVanilla);
                Selections.insertObject(cursde, cd);
            }
        }
    }
}


void
cGroupDesc::select_unassoc_psubs()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (!s->dual()) {
                s->cdesc()->set_state(CDVanilla);
                Selections.insertObject(cursdp, s->cdesc());
            }
        }
    }
}


void
cGroupDesc::select_unassoc_esubs()
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde || cursde->isSymbolic())
        return;
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sEinstList *s = sl->esubs(); s; s = s->next()) {
            if (!s->dual_subc()) {
                s->cdesc()->set_state(CDVanilla);
                Selections.insertObject(cursde, s->cdesc());
            }
        }
    }
    if (gd_extra_subs) {
        for (sEinstList *s = gd_extra_subs; s; s = s->next()) {
            s->cdesc()->set_state(CDVanilla);
            Selections.insertObject(cursde, s->cdesc());
        }
    }
}


//
// Private cGroupDesc functions
//

namespace {
    // Return true if the (electrical) object is a wire capacitor.  This
    // is indicated by a special name prefix.
    //
    bool is_wire_cap(CDc *cd)
    {
        CDp_name *pn = (CDp_name*)cd->prpty(P_NAME);
        if (!pn)
            return (false);
        if (pn->assigned_name() &&
                lstring::prefix(WIRECAP_PREFIX, pn->assigned_name()))
            return (true);
        return (false);
    }


    // Return true if the node lists are the same, takes account of
    // two permutable terminals.
    //
    bool parallel(unsigned int sz, const int *permutes,
        const CDp_cnode *const *ary1, const CDp_cnode *const *ary2)
    {
        if (permutes) {
            unsigned int i = permutes[0];
            unsigned int j = permutes[1];
            if (i >= sz || j >= sz)
                return (false);
            if (!ary1[i] || !ary2[i] || !ary1[j] || !ary2[j])
                return (false);
            if (!((ary1[i]->enode() == ary2[i]->enode() &&
                    ary1[j]->enode() == ary2[j]->enode()) ||
                    (ary1[i]->enode() == ary2[j]->enode() &&
                    ary1[j]->enode() == ary2[i]->enode())))
                return (false);
        }
        for (int i = 0; i < (int)sz; i++) {
            if (permutes && (i == permutes[0] || i == permutes[1]))
                continue;
            if (ary1[i] && ary2[i]) {
                if (ary1[i]->enode() != ary2[i]->enode())
                    return (false);
                continue;
            }
            if (!ary1[i] && !ary2[i])
                continue;
            return (false);
        }
        return (true);
    }
}


XIrt
cGroupDesc::set_duality_first_pass()
{
    CDcbin cbin(gd_celldesc);
    if (!cbin.elec() || cbin.elec()->isEmpty()) {
        // If there is not electrical part we can skip further
        // processing.  We'll do this silently.

        clear_duality();
        return (XIok);
    }

#ifdef TIME_DBG
#ifdef TIME_DBG_XTRA
    Tdbg()->start_timing("first_pass_setup1");
#else
    Tdbg()->accum_timing("first_pass_setup");
#endif
#endif
    if (ExtErrLog.log_associating() && ExtErrLog.log_fp()) {
        FILE *fp = ExtErrLog.log_fp();
        fprintf(fp,
            "\n=======================================================\n");
        fprintf(fp, "Pre-Associating cell %s\n",
            Tstring(gd_celldesc->cellname()));
    }
tiptop:
    clear_duality();

    CDs *esdesc = cbin.elec();

    // List all electrical subcells and devices.
    sEinstList *dlist = 0, *slist = 0;
    gd_etlist = new sElecNetList(esdesc);  // This calls cSced::connect().
    if (!gd_etlist) {
        // Nothing to do.
        ExtErrLog.add_log(ExtLogAssoc, "No electrical nets found, quitting.");
        return (XIok);
    }
#ifdef TIME_DBG_XTRA
    Tdbg()->accum_timing("first_pass_setup1");
    Tdbg()->start_timing("first_pass_setup2");
#endif

    gd_etlist->list_devs_and_subs(&dlist, &slist);

    // This will pop up an error message, and association will likely
    // fail, but soldier on...
    check_series_merge();

#ifdef TIME_DBG_XTRA
    Tdbg()->accum_timing("first_pass_setup2");
    Tdbg()->start_timing("first_pass_setup3");
#endif

    link_elec_subs(slist);
    link_elec_devs(dlist);

#ifdef TIME_DBG_XTRA
    Tdbg()->accum_timing("first_pass_setup3");
    Tdbg()->start_timing("first_pass_setup4");
#endif

top:
    // Smash in the left-over electrical subcircuit calls.
    while (gd_extra_subs) {
        sEinstList *s = gd_extra_subs;
        gd_extra_subs = s->next();
        sEinstList *devs = 0;
        sEinstList *subs = 0;
        gd_etlist->flatten(s->cdesc(), s->cdesc_index(), &devs, &subs);
        link_elec_subs(subs);
        link_elec_devs(devs);
        ExtErrLog.add_log(ExtLogAssoc, "Smashing %s %d into %s schematic.",
            Tstring(s->cdesc()->cellname()), s->cdesc_index(),
            Tstring(gd_celldesc->cellname()));
        delete s;
    }

    combine_elec_parallel();

#ifdef TIME_DBG_XTRA
    Tdbg()->accum_timing("first_pass_setup4");
    Tdbg()->start_timing("first_pass_setup5");
#endif

    // Set the electrical node property index number in the contact
    // descriptions.  This will speed access in the tests.

    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            sDevContactDesc *cd = p->devs()->desc()->contacts();
            if (!cd->setup_contact_indices(p->devs()->desc())) {
                ExtErrLog.add_log(ExtLogAssoc, "Setup error: %s",
                    Errs()->get_error());
                return (XIbad);
            }
        }
    }

#ifdef TIME_DBG_XTRA
    Tdbg()->accum_timing("first_pass_setup5");
    Tdbg()->start_timing("first_pass_setup6");
#endif

    // Mark the groups that are cell connections, as known from
    // extraction at higher levels.

    sGdList *gdl = EX()->referenceList(gd_celldesc);
    for ( ; gdl; gdl = gdl->next) {
        cGroupDesc *pgd = gdl->gd;
        for (sSubcList *sl = pgd->subckts(); sl; sl = sl->next()) {
            sSubcInst *si = sl->subs();
            if (!si)
                continue;
            if (si->cdesc()->cellname() != gd_celldesc->cellname())
                continue;
            for ( ; si; si = si->next()) {
                sSubcContactInst *ci = si->contacts();
                for ( ; ci; ci = ci->next())
                    gd_groups[ci->subc_group()].set_cell_connection(true);
            }
        }
    }

    if (ExtErrLog.log_associating()) {
        CDpin *pins = list_cell_terms();
        for (CDpin *p = pins; p; p = p->next()) {
            CDsterm *term = p->term();
            ExtErrLog.add_log(ExtLogAssoc,
                "Formal terminal %s, group %d, %d,%d %s.",
                term->name(), term->group(), term->lx(), term->ly(),
                term->is_fixed() ? "(fixed)" : "");
        }
        CDpin::destroy(pins);
    }

    // Sort the devices and subcircuits by increasing instantiation
    // count.  We guess that it might be easier to associate fewer
    // objects, and we do the easier cases first.

    sort_devs(true);
    sort_subs(true);

#ifdef TIME_DBG
#ifdef TIME_DBG_XTRA
    Tdbg()->accum_timing("first_pass_setup6");
#else
    Tdbg()->accum_timing("first_pass_setup");
#endif
    Tdbg()->start_timing("first_pass_solve");
#endif

    // The rest is automatic.
    try {
        gd_discreps = solve_duals();
    }
    catch (XIrt) {
        ExtErrLog.add_log(ExtLogAssoc, "Caught exception, quitting.");
        return (XIbad);
    }

    // It there are physical subcircuits where all electrical
    // subcircuits of the same type have been associated, go back and
    // flatten these, and re-associate.

    bool smashed = false;
    if (!flatten(&smashed, true)) {
        ExtErrLog.add_err("In %s, physical flatten failed while associating.",
            Tstring(gd_celldesc->cellname()));
        return (XIbad);
    }
    if (smashed) {

        // Re-do the subcircuit numbering.
        sort_and_renumber_subs();
        sort_subs(true);

        // Do this again, probably not necessary.
        for (int i = 0; i < gd_asize; i++) {
            if (gd_groups[i].net())
                gd_groups[i].net()->set_length();
        }

        // Sort and renumber the devices, then combine parallel.
        sort_and_renumber_devs();
        sort_devs(true);
        combine_devices();

        // We basically start over with association of this cell.
        goto tiptop;
    }

    // If all physical subcircuits have been associated but there are
    // "left over" electrical subcircuits, flatten these and associate
    // again.

    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        bool alldone = true;
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (!s->dual()) {
                alldone = false;
                break;
            }
        }
        if (alldone) {
            sEinstList *ep = 0, *en;
            for (sEinstList *s = sl->esubs(); s; s = en) {
                en = s->next();
                if (!s->dual_subc()) {
                    gd_etlist->remove(s->cdesc(), s->cdesc_index());
                    sEinstList *devs = 0;
                    sEinstList *subs = 0;
                    gd_etlist->flatten(s->cdesc(), s->cdesc_index(),
                        &devs, &subs);
                    if (ep)
                        ep->set_next(en);
                    else
                        sl->set_esubs(en);
                    link_elec_subs(subs);
                    link_elec_devs(devs);
                    smashed = true;
                    ExtErrLog.add_log(ExtLogAssoc,
                        "Smashing %s %d into %s schematic.",
                        Tstring(s->cdesc()->cellname()), s->cdesc_index(),
                        Tstring(gd_celldesc->cellname()));
                    delete s;
                    continue;
                }
                ep = s;
            }
        }

    }
    if (smashed)
        goto top;

#ifdef TIME_DBG
    Tdbg()->accum_timing("first_pass_solve");
    Tdbg()->start_timing("first_pass_misc");
#endif

    // Set the cpnode of each sDevContactInst, and set terminal marker
    // locations.
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next())
                ident_term_nodes(di);
        }
    }

    reposition_terminals(true);
    cTfmStack stk;
    position_labels(&stk);

    if (ExtErrLog.log_associating()) {

        if (gd_discreps) {
            // Something physical didn't associate, count errors.
            ExtErrLog.add_log(ExtLogAssoc,
                "Residual physical nonassociations:");
            for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
                for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                    if (!s->dual()) {
                        char *iname = s->instance_name();
                        ExtErrLog.add_log(ExtLogAssoc, "  subcircuit %s",
                            iname);
                        delete [] iname;
                    }
                }
            }
            for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
                for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                    for (sDevInst *s = p->devs(); s; s = s->next()) {
                        if (!s->dual()) {
                            ExtErrLog.add_log(ExtLogAssoc, "  device %s %d",
                                s->desc()->name(), s->index());
                        }
                    }
                }
            }
            int psize = nextindex();
            for (int i = 1; i < psize; i++) {
                if (has_terms(i) && gd_groups[i].node() < 0 &&
                        !check_global(i) && !check_unas_wire_only(i))
                    ExtErrLog.add_log(ExtLogAssoc, "  group %d", i);
            }
        }
        else {
            ExtErrLog.add_log(ExtLogAssoc,
                "All physical devices, subcircuits, and groups "
                "have been associated.");
        }

        // Check if all electrical objects have been associated.
        const char *msg = "Residual electrical nonassociations:";
        bool pr = false;
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            for (sEinstList *s = sl->esubs(); s; s = s->next())
                if (!s->dual_subc()) {
                    if (!pr) {
                        ExtErrLog.add_log(ExtLogAssoc, msg);
                        pr = true;
                    }
                    char *instname =
                        s->cdesc()->getElecInstName(s->cdesc_index());
                    ExtErrLog.add_log(ExtLogAssoc, "  subcircuit %s %s",
                        Tstring(s->cdesc()->cellname()), instname);
                    delete [] instname;
                }
        }
        for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
            for (sEinstList *s = dv->edevs(); s; s = s->next()) {
                if (!s->dual_dev()) {
                    if (!pr) {
                        ExtErrLog.add_log(ExtLogAssoc, msg);
                        pr = true;
                    }
                    char *instname =
                        s->cdesc()->getElecInstName(s->cdesc_index());
                    ExtErrLog.add_log(ExtLogAssoc, "  device %s %s",
                        Tstring(s->cdesc()->cellname()), instname);
                    delete [] instname;
                }
            }
        }
        for (int i = 1; i < gd_etlist->size(); i++) {
            if ((gd_etlist->pins_of_node(i) || gd_etlist->conts_of_node(i)) &&
                    gd_etlist->group_of_node(i) < 0) {
                if (!pr) {
                    ExtErrLog.add_log(ExtLogAssoc, msg);
                    pr = true;
                }
                ExtErrLog.add_log(ExtLogAssoc, "  net %d", i);
            }
        }
        if (!pr) {
            ExtErrLog.add_log(ExtLogAssoc,
                "All electrical devices, subcircuits, and nets have "
                "been associated.");
        }
    }
#ifdef TIME_DBG
    Tdbg()->accum_timing("first_pass_misc");
#endif
    return (XIok);
}


// Work function to actually establish duality for the cell.
//
XIrt
cGroupDesc::set_duality()
{
    if (!gd_etlist) {
        // If there is not electrical part we can skip further
        // processing.  We'll do this silently.

        clear_duality();
        gd_celldesc->setAssociated(true);
        return (XIok);
    }

    if (ExtErrLog.log_associating() && ExtErrLog.log_fp()) {
        FILE *fp = ExtErrLog.log_fp();
        fprintf(fp,
            "\n=======================================================\n");
        fprintf(fp, "Associating cell %s\n",
            Tstring(gd_celldesc->cellname()));
    }

    try {
        gd_discreps = solve_duals();
    }
    catch (XIrt) {
        ExtErrLog.add_log(ExtLogAssoc, "Caught exception, quitting.");
        return (XIbad);
    }

    // Set the cpnode of each sDevContactInst, and set terminal marker
    // locations.
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next())
                ident_term_nodes(di);
        }
    }

    reposition_terminals();
    cTfmStack stk;
    position_labels(&stk);

    if (ExtErrLog.log_associating()) {

        if (ExtErrLog.verbose()) {
            // Print a listing of the devices.

            for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
                ExtErrLog.add_log(ExtLogAssoc, "%s",
                    dv->prefixes()->devs()->desc()->name());
                char buf[256];
                for (sEinstList *el = dv->edevs(); el; el = el->next()) {
                    char *instname = el->instance_name();
                    sprintf(buf, "  %-16s", instname);
                    delete [] instname;
                    CDp_cnode *pc = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
                    for ( ; pc; pc = pc->next()) {
                        CDp_cnode *pc1 = pc;
                        if (el->cdesc_index() > 0) {
                            CDp_range *pr =
                                (CDp_range*)el->cdesc()->prpty(P_RANGE);
                            if (pr)
                                pc1 = pr->node(0, el->cdesc_index(),
                                    pc->index());
                        }
                        sprintf(buf + strlen(buf), " %8s %3d",
                            Tstring(pc->term_name()),
                            pc1 ? pc1->enode() : pc->enode());
                    }
                    ExtErrLog.add_log(ExtLogAssoc, buf);
                }
            }
        }

        // subckt list match?
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            int np = 0, ne = 0;
            for (sSubcInst *s = sl->subs(); s; s = s->next(), np++) ;
            for (sEinstList *c = sl->esubs(); c; c = c->next(), ne++) ;
            if (np != ne) {
                ExtErrLog.add_log(ExtLogAssoc,
                "Subcircuit %s count mismatch: %d physical, %d electrical.",
                    Tstring(sl->subs()->cdesc()->cellname()), np, ne);
            }
        }

        // device list match?
        for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
            int np = 0, ne = 0;
            for (sDevPrefixList *p = dv->prefixes(); p; p = p->next())
                for (sDevInst *s = p->devs(); s; s = s->next(), np++) ;
            for (sEinstList *c = dv->edevs(); c; c = c->next(), ne++) ;
            if (np != ne) {
                ExtErrLog.add_log(ExtLogAssoc,
                    "Device %s count mismatch: %d physical, %d electrical.",
                    dv->devname(), np, ne);
            }
        }

        // net count match?
        int ncnt = gd_etlist->count_active();
        int gcnt = nextindex() - 1;
        if (ncnt != gcnt) {
            ExtErrLog.add_log(ExtLogAssoc,
                "Net count mismatch: %d physical, %d electrical.",
                gcnt, ncnt);
        }

        if (gd_discreps) {
            // Something physical didn't associate, count errors.
            ExtErrLog.add_log(ExtLogAssoc,
                "Residual physical nonassociations:");
            for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
                for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                    if (!s->dual()) {
                        char *iname = s->instance_name();
                        ExtErrLog.add_log(ExtLogAssoc, "  subcircuit %s",
                            iname);
                        delete [] iname;
                    }
                }
            }
            for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
                for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                    for (sDevInst *s = p->devs(); s; s = s->next()) {
                        if (!s->dual()) {
                            ExtErrLog.add_log(ExtLogAssoc, "  device %s %d",
                                s->desc()->name(), s->index());
                        }
                    }
                }
            }
            int psize = nextindex();
            for (int i = 1; i < psize; i++) {
                if (has_terms(i) && gd_groups[i].node() < 0 &&
                        !check_global(i) && !check_unas_wire_only(i))
                    ExtErrLog.add_log(ExtLogAssoc, "  group %d", i);
            }
        }
        else {
            ExtErrLog.add_log(ExtLogAssoc,
                "All physical devices, subcircuits, and groups "
                "have been associated.");
        }

        // Check if all electrical objects have been associated.
        const char *msg = "Residual electrical nonassociations:";
        bool pr = false;
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            for (sEinstList *s = sl->esubs(); s; s = s->next())
                if (!s->dual_subc()) {
                    if (!pr) {
                        ExtErrLog.add_log(ExtLogAssoc, msg);
                        pr = true;
                    }
                    char *instname =
                        s->cdesc()->getElecInstName(s->cdesc_index());
                    ExtErrLog.add_log(ExtLogAssoc, "  subcircuit %s %s",
                        Tstring(s->cdesc()->cellname()), instname);
                    delete [] instname;
                }
        }
        for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
            for (sEinstList *s = dv->edevs(); s; s = s->next()) {
                if (!s->dual_dev()) {
                    if (!pr) {
                        ExtErrLog.add_log(ExtLogAssoc, msg);
                        pr = true;
                    }
                    char *instname =
                        s->cdesc()->getElecInstName(s->cdesc_index());
                    ExtErrLog.add_log(ExtLogAssoc, "  device %s %s",
                        Tstring(s->cdesc()->cellname()), instname);
                    delete [] instname;
                }
            }
        }
        for (int i = 1; i < gd_etlist->size(); i++) {
            if ((gd_etlist->pins_of_node(i) || gd_etlist->conts_of_node(i)) &&
                    gd_etlist->group_of_node(i) < 0) {
                if (!pr) {
                    ExtErrLog.add_log(ExtLogAssoc, msg);
                    pr = true;
                }
                ExtErrLog.add_log(ExtLogAssoc, "  net %d", i);
            }
        }
        if (!pr) {
            ExtErrLog.add_log(ExtLogAssoc,
                "All electrical devices, subcircuits, and nets have "
                "been associated.");
        }
    }
    gd_celldesc->setAssociated(true);
    return (XIok);
}


// If this returns true, all is well.  Otherwise, there is a cell
// terminal connected to an electrical net consisting only of two or
// more device terminals, from the same type of device, that is
// series-mergeable.  In that case, the terminal must have the fixed
// flag set, so that the terminal placement is known during grouping,
// when series merging takes place.  Without the terminal connection,
// the intermediate node will likely be merged away!  Obviously,
// association will fail in that case.
//
bool
cGroupDesc::check_series_merge()
{
    if (EX()->isNoMergeSeries())
        return (true);

    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    if (!esdesc)
        return (true);
    CDp_snode *ps = (CDp_snode*)esdesc->prpty(P_NODE);
    for ( ; ps; ps = ps->next()) {
        if (!ps->cell_terminal())
            continue;
        if (ps->cell_terminal()->is_fixed())
            continue;
        CDcont *tl = conts_of_node(ps->enode());
        CDs *msd = 0;
        for (CDcont *t = tl; t; t = t->next()) {
            CDc *cd = t->term()->instance();
            if (!cd)
                continue;
            CDs *sd = cd->masterCell();
            if (!sd)
                continue;
            if (!msd)
                msd = sd;
            else if (msd != sd) {
                msd = 0;
                break;
            }
        }
        if (!msd || !msd->isDevice())
            continue;
        for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
            if (dv->devname() != msd->cellname())
                continue;
            for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                sDevInst *s = p->devs();
                if (s && s->desc()->merge_series()) {
                    ExtErrLog.add_err(
                        "In %s, terminal %s must have the FIXED flag set, as\n"
                        "it connects only to an intermediate node of a "
                        "series-mergeable device.\n",
                        Tstring(gd_celldesc->cellname()), ps->term_name());
                    return (false);
                }
            }
            break;
        }
    }
    return (true);
}


// Remove from slist the subcircuits that match by name a subcircuit
// in the physical part and add them to the subcircuits listing
// element.
//
void
cGroupDesc::link_elec_subs(sEinstList *slist)
{
    if (!slist)
        return;
   
    for (sSubcList *su = gd_subckts; su; su = su->next()) {
        CDcellName name = su->subs()->cdesc()->cellname();
        sEinstList *sp = 0, *sn;
        for (sEinstList *s = slist; s; s = sn) {
            sn = s->next();
            if (s->cdesc()->cellname() == name) {
                if (!sp)
                    slist = sn;
                else
                    sp->set_next(sn);
                s->set_next(su->esubs());
                su->set_esubs(s);
                continue;
            }
            sp = s;
        }
    }
    if (slist) {
        // Keep left over subs in extra_subs.  These do not match,
        // by name, any physical subcircuit.

        if (ExtErrLog.log_associating()) {
            ExtErrLog.add_log(ExtLogAssoc, "Extra electrical subckts:");
            for (sEinstList *s = slist; s; s = s->next()) {
                ExtErrLog.add_log(ExtLogAssoc, "  Instance of %s",
                    Tstring(s->cdesc()->cellname()));
            }
        }
        if (!gd_extra_subs)
            gd_extra_subs = slist;
        else {
            sEinstList *e = gd_extra_subs;
            while (e->next())
                e = e->next();
            e->set_next(slist);
        }
    }
}


// Remove from dlist devices that match by cellname extracted physical
// devices, and link them into the device listing element.
//
void
cGroupDesc::link_elec_devs(sEinstList *dlist)
{
    if (!dlist)
        return;

    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        sEinstList *dp = 0, *dn;
        for (sEinstList *d = dlist; d; d = dn) {
            dn = d->next();
            // Don't include wire capacitors.
            if (dv->devname() == d->cdesc()->cellname() &&
                    !is_wire_cap(d->cdesc())) {
                if (!dp)
                    dlist = dn;
                else
                    dp->set_next(dn);
                d->set_next(dv->edevs());
                dv->set_edevs(d);
                continue;
            }
            dp = d;
        }
    }
    if (dlist) {
        // Should only be gnd, terminal, and wire cap devices left.
        // Keep left over devs in extra_devs.

        bool printed = false;
        sEinstList *dp = 0, *dn;
        for (sEinstList *d = dlist; d; d = dn) {
            dn = d->next();
            CDelecCellType tp = d->cdesc()->elecCellType();
            if (tp == CDelecDev) {
                if (!printed) {
                    ExtErrLog.add_log(ExtLogAssoc,
                        "Extra electrical devices:");
                    printed = true;
                }
                ExtErrLog.add_log(ExtLogAssoc, "  Instance of %s",
                    Tstring(d->cdesc()->cellname()));
                if (!dp)
                    dlist = dn;
                else
                    dp->set_next(dn);
                d->set_next(gd_extra_devs);
                gd_extra_devs = d;
                continue;
            }
            dp = d;
        }
        sEinstList::destroy(dlist);  // gnd and terminal only
    }
}


// If parallel merging,combine parallel-connected devices.  It is
// crucial that the physical and electrical device counts be the same.
//
void
cGroupDesc::combine_elec_parallel()
{
    SymTab *ntab = 0;   // nodes containing dead terminals
    SymTab *ttab = 0;   // dead terminals

    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        if (!dv->prefixes()->devs()->desc()->merge_parallel())
            continue;

        // First sort.  The main reason for this is so that for
        // vectored devices, the index 0 device will be seen first,
        // and if the vectored elements are connected in parallel, the
        // index 0 device will remain.

        dv->set_edevs(sEinstList::sort(dv->edevs()));

        for (sEinstList *c1 = dv->edevs(); c1;  c1 = c1->next()) {
            unsigned int asz1;
            const CDp_cnode *const *ary1 = c1->nodes(&asz1);
            if (!ary1)
                continue;
            int prms[2];
            const int *permutes =
                c1->permutes(dv->prefixes()->devs()->desc(), prms);

            int pcnt = 0;
            sEinstList *ep = c1, *en;
            for (sEinstList *c2 = c1->next(); c2;  c2 = en) {
                en = c2->next();
                unsigned int asz2;
                const CDp_cnode *const *ary2 = c2->nodes(&asz2);
                if (ary2 && asz2 == asz1 &&
                        parallel(asz1, permutes, ary1, ary2)) {
                    ep->set_next(en);
                    c2->set_next(c1->sections());
                    c1->set_sections(c2);

                    if (!ttab)
                        ttab = new SymTab(false, false);
                    for (unsigned int i = 0; i < asz2; i++) {
                        if (ary2[i]->inst_terminal()) {
                            ttab->add((unsigned long)ary2[i]->inst_terminal(),
                                0, false);
                        }
                    }
                    delete [] ary2;

                    pcnt++;
                    continue;
                }
                delete [] ary2;
                ep = c2;
            }
            if (pcnt) {
                if (!ntab)
                    ntab = new SymTab(false, false);
                for (unsigned int i = 0; i < asz1; i++)
                    ntab->add((unsigned long)ary1[i]->enode(), 0, true);
            }
            delete [] ary1;
        }
    }
    // Remove the corresponding terminals from the lists.
    if (ntab)
        gd_etlist->purge_terminals(ttab, ntab);
    delete ttab;
    delete ntab;
}


// Move cell terminals into position if necessary.
//
void
cGroupDesc::reposition_terminals(bool no_errs)
{
    // If the present cell is not the top level cell and isn't
    // instantiated in the hierarchy, all instances must have been
    // flattened.  Skip this, since we don't care about terminal
    // locations, an attempted placement can produce errors.

    if (!top_level() && !EX()->referenceList(gd_celldesc))
        return;

    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    if (!esdesc)
        return;
    gd_etlist->update_pins();
    CDp_snode *ps = (CDp_snode*)esdesc->prpty(P_NODE);
    for ( ; ps; ps = ps->next()) {
        CDsterm *psterm = ps->cell_terminal();
        if (!psterm)
            continue;
        int node = ps->enode();
        if (node < 0)
            continue;
        int group = group_of_node(node);
        if (group < 0 && ps->term_name()) {
            // If the node is not associated, try to resolve by name. 
            // We should need to do this only when the net is
            // wire-only.

            for (int i = 0; i < gd_asize; i++) {
                if (ps->term_name() == gd_groups[i].netname()) {
                    group = i;
                    break;
                }
            }
        }
        if (group >= 0) {
            int ogrp = psterm->group();
            if (ogrp != group) {
                // Hmmmm.  Association tells us that this terminal was
                // initialized in the wrong group.  Move to the new
                // group.
                ExtErrLog.add_log(ExtLogAssoc,
                    "After association, moving terminal %s from group %d "
                    "to group %d.", psterm->name(), ogrp, group);
                if (!bind_term_to_group(psterm, group)) {
                    if (no_errs) {
                        ExtErrLog.add_log(ExtLogAssoc,
                            "In %s, failed to bind %s %s to group %d.",
                            Tstring(gd_celldesc->cellname()),
                            gd_groups[group].net() ?
                                "terminal" : "virtual terminal",
                            psterm->name(), group);
                    }
                    else {
                        ExtErrLog.add_err(
                            "In %s, failed to bind %s %s to group %d.",
                            Tstring(gd_celldesc->cellname()),
                            gd_groups[group].net() ?
                                "terminal" : "virtual terminal",
                            psterm->name(), group);
                    }
                }
            }
            if (psterm->group() == group) {
                // Make sure that terminal is in termlist once.
                int cnt = 0;
                for (CDpin *p = gd_groups[group].termlist(); p; p = p->next()) {
                    if (p->term() == psterm)
                        cnt++;
                }
                if (cnt > 1) {
                    ExtErrLog.add_log(ExtLogAssoc,
                        "Fixed error,, terminal %s was listed in "
                        "group %d %d times.",
                        psterm->name(), group, cnt);

                    CDpin *pp = 0, *pn;
                    CDpin *p = gd_groups[group].termlist();
                    for ( ; p; p = pn) {
                        pn = p->next();
                        if (p->term() == psterm) {
                            if (pp)
                                pp->set_next(pn);
                            else
                                gd_groups[group].set_termlist(pn);
                            delete p;
                            cnt--;
                            if (cnt == 1)
                                break;
                        }
                        pp = p;
                    }
                }
                else if (!cnt)
                    bind_term(psterm, group);
            }
            continue;
        }

        // Presently, the terminal group is unassigned.  Unless the
        // location is fixed, we'll move the terminal to a correct
        // position.

        if (psterm->is_fixed()) {
            if (no_errs) {
                ExtErrLog.add_log(ExtLogAssoc,
                    "In %s, failed to bind terminal %s to a group,\n"
                    "as the FIXED flag is set but no group is resolved "
                    "at the location.", Tstring(gd_celldesc->cellname()),
                    psterm->name());
            }
            else {
                ExtErrLog.add_err(
                    "In %s, failed to bind terminal %s to a group,\n"
                    "as the FIXED flag is set but no group is resolved "
                    "at the location.", Tstring(gd_celldesc->cellname()),
                    psterm->name());
            }
            continue;
        }

        // Find a subcircuit instance terminal to link to.
        CDsterm *mterm = 0;
        CDc *cdesc = 0;
        int cdesc_vecix = 0;
        for (CDcont *t = conts_of_node(node); t; t = t->next()) {
            if (!t->term()->instance())
                continue;

            CDc *cd = t->term()->instance();
            CDs *msdesc = cd->masterCell(true);
            if (!msdesc || msdesc->isDevice())
                continue;
            CDsterm *mt = t->term()->master_term();
            if (!mt)
                continue;
            if (mt->group() < 0)
                continue;

            // OK, we've got an instance terminal with a corresponding
            // master terminal that has a group assignment.

            mterm = mt;
            cdesc = cd;
            cdesc_vecix = t->term()->inst_index();
            break;
        }
        if (!cdesc) {
            ExtErrLog.add_log(ExtLogExt,
                "Failed to bind terminal %s to a group, no connected\n"
                "subcircuit or device.\n", psterm->name());
            continue;
        }

        // The cdesc is a subcircuit, take the terminal as a virtual
        // contact to this subcircuit, assign a new group, and move
        // the terminal into position.

        sSubcInst *subc = find_dual_subc(cdesc, cdesc_vecix);
        if (!subc) {
            ExtErrLog.add_log(ExtLogExt,
                "Failed to bind terminal %s to a group, no dual for\n"
                "connected subcircuit.\n", psterm->name());
            continue;
        }
        int mg = mterm->group();
        int pg = -1;
        for (sSubcContactInst *ci = subc->contacts(); ci; ci = ci->next()) {
            if (ci->subc_group() == mg) {
                pg = ci->parent_group();
                break;
            }
        }

        if (pg > 0)
            group = pg;
        else {
            group = nextindex();
            alloc_groups(group + 1);
            sSubcContactInst *ci = subc->add(group, mterm->group());
            gd_groups[group].set_subc_contacts(
                new sSubcContactList(ci, gd_groups[group].subc_contacts()));
        }
        psterm->set_v_group(group);
        bind_term(psterm, group);
        gd_groups[group].set_node(node);
        gd_etlist->set_group(node, group);

        int x = mterm->lx();
        int y = mterm->ly();
        cTfmStack stk;
        stk.TPush();
        stk.TApplyTransform(subc->cdesc());
        CDap ap(subc->cdesc());
        stk.TTransMult(subc->ix()*ap.dx, subc->iy()*ap.dy);
        stk.TPoint(&x, &y);
        stk.TPop();
        psterm->set_loc(x, y);
        psterm->set_layer(mterm->layer());

        ExtErrLog.add_log(ExtLogAssocV,
            "reposition_terminals %s grp=%d x=%d y=%d.", psterm->name(),
            group, x, y);
    }
}


// Move psterm to a location for it to be bound to the given group,
// which has no metal at the top level.  If we find a subcircuit
// instance terminal in the group, place psterm at the same location,
// and copy the layer.
//
bool
cGroupDesc::bind_term_to_vgroup(CDsterm *psterm, int group)
{
    if (!psterm || psterm->instance())
        return (false);
    if (psterm->is_fixed())
        return (false);
    if (gd_groups[group].net())
        return (false);  // not virtual
    if (group < 0 || group >= gd_asize)
        return (false);

    // If the terminal is already bound to a group, unbind it.
    int ogrp = psterm->group();
    if (ogrp >= 0) {
        if (ogrp == group)
            return (true);

        psterm->set_ref(0);
        CDpin *pp = 0, *pn;
        for (CDpin *p = gd_groups[ogrp].termlist(); p; p = pn) {
            pn = p->next();
            if (p->term() == psterm) {
                if (pp)
                    pp->set_next(pn);
                else
                    gd_groups[ogrp].set_termlist(pn);
                delete p;
                break;
            }
            pp = p;
        }
    }

    // Keep this, so that if placement fails, the term->group() method
    // is correct.
    psterm->set_v_group(group);

    for (sSubcContactList *sl = gd_groups[group].subc_contacts(); sl;
            sl = sl->next()) {
        sSubcContactInst *c = sl->contact();
        sSubcInst *subc = c->subc();
        if (!subc->dual())
            continue;
        CDs *pmsd = subc->cdesc()->masterCell();
        cGroupDesc *gd = pmsd ? pmsd->groups() : 0;
        if (!gd)
            continue;
        sGroup *gs = gd->group_for(c->subc_group());
        if (!gs || gs->node() < 0 || !gs->termlist())
            continue;
        CDsterm *mterm = gs->termlist()->term();

        int x = mterm->lx();
        int y = mterm->ly();
        cTfmStack stk;
        stk.TPush();
        stk.TApplyTransform(subc->cdesc());
        CDap ap(subc->cdesc());
        stk.TTransMult(subc->ix()*ap.dx, subc->iy()*ap.dy);
        stk.TPoint(&x, &y);
        stk.TPop();
        psterm->set_loc(x, y);
        psterm->set_v_group(group);
        psterm->set_layer(mterm->layer());

        bind_term(psterm, group);
        CDp_snode *psx = psterm->node_prpty();
        if (psx) {
            int node = psx->enode();
            gd_groups[group].set_node(node);
            gd_etlist->set_group(node, group);
        }

        ExtErrLog.add_log(ExtLogAssocV,
            "bind_term_to_vgroup %s grp=%d x=%d y=%d.", psterm->name(),
            group, x, y);

        return (true);
    }
    return (false);
}


// Set the location for the subcell name markers and instance
// terminals.
//
void
cGroupDesc::position_labels(cTfmStack *tstk)
{
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (!s->dual())
                continue;

            sEinstList *edual = s->dual();
            CDp_name *pn = 0;
            if (edual->cdesc_index() == 0)
                pn = (CDp_name*)edual->cdesc()->prpty(P_NAME);
            else {
                CDp_range *pr = (CDp_range*)edual->cdesc()->prpty(P_RANGE);
                if (pr)
                    pn = pr->name_prp(0, edual->cdesc_index());
            }

            if (pn) {
                BBox sBB = *s->cdesc()->masterCell(true)->BB();
                tstk->TPush();
                tstk->TApplyTransform(s->cdesc());
                tstk->TPremultiply();
                CDap ap(s->cdesc());
                tstk->TTransMult(s->ix()*ap.dx, s->iy()*ap.dy);
                tstk->TBB(&sBB, 0);
                pn->set_pos_x((sBB.left + sBB.right)/2);
                pn->set_pos_y((sBB.bottom + sBB.top)/2);
                pn->set_located(true);
                tstk->TPop();
            }
            // Set the loctions for the instance terminals.
            s->place_phys_terminals();
        }
    }
}


// Recursive tail for the subcircuit_permutation_fix function.  We
// know that the group descriptors have the hint_node field set.  All
// objects are associated, with terminal reference errors only.  We
// are going to switch to the new node numbers, recursively.  This is
// actually called whether or not there errors (nfixes is the error
// count), since there may be errors at a lower level.
//
void
cGroupDesc::subcircuit_permutation_fix_rc(int nfixes)
{
    // For all groups, establish deality to the hint node, clear the
    // hint node.
    if (nfixes > 0) {
        for (int i = 1; i < gd_asize; i++) {
            sGroup &g = gd_groups[i];
            if (g.hint_node() >= 0) {
                g.set_node(g.hint_node());
                g.set_hint_node(-1);
                gd_etlist->set_group(g.node(), i);
            }
        }
        // Move terminals to correct positions.
        reposition_terminals();

        // Blow away the duality of devices and subcircuits.
        for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
            for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                for (sDevInst *di = p->devs(); di; di = di->next())
                    di->set_dual(0);
            }
            for (sEinstList *e = dv->edevs(); e; e = e->next())
                e->set_dual((sDevInst*)0);
        }
        for (sSubcList *su = gd_subckts; su; su = su->next()) {
            for (sSubcInst *s = su->subs(); s; s = s->next())
                s->set_dual(0);
            for (sEinstList *e = su->esubs(); e; e = e->next())
                e->set_dual((sSubcInst*)0);
        }

        // Recompute the subcircuit duality, using a different algorithm
        // than normal, which does not rely on subcircuits being correctly
        // associated, and uses that all groups are associated.

        for (sSubcList *sl = gd_subckts; sl; sl = sl->next())
            ident_subckt(sl, -1, false, true);
    }

    // Run the permutation fix on the subcircuits.
    for (sSubcList *su = gd_subckts; su; su = su->next())
        subcircuit_permutation_fix(su);

    // Run the solver again, which we can do since subcircuits are now
    // correctly associated, to associate the devices.

    solve_duals();
}


// Attempt to associate the given group to the corresponding
// electrical node, return true if association established.
//
bool
cGroupDesc::ident_node(int grp)
{
    if (!has_net_or_terms(grp) || gd_groups[grp].node() >= 0)
        // Empty group, or already associated.
        return (false);

    // Find the node with the largest comparison value to this group.
    // If not unique, we pass.

    int jlast = -1;
    int mx = -1;
    for (int j = 0; j < gd_etlist->size(); j++) {
        int n = ep_grp_comp(grp, j);
        if (n > mx) {
            mx = n;
            jlast = j;
        }
        else if (n == mx) {
            // The grp_break is set when all devices and
            // subcircuits have been associated.  In that case we can
            // break the symmetry.

            if (!grp_break())
                jlast = -1;
        }
    }
    if (jlast < 0)
        return (false);

    // Now compare the node to the other groups, if we find a
    // comparison value equal or larger, pass.

    int psize = nextindex();
    for (int k = 1; k < psize; k++) {
        if (has_net_or_terms(k) && gd_groups[k].node() < 0 && k != grp) {
            if (ep_grp_comp(k, jlast) >= mx) {
                return (false);
            }
        }
    }

    gd_groups[grp].set_node(jlast);
    gd_etlist->set_group(jlast, grp);
    if (ExtErrLog.log_associating()) {
        const char *msg = gd_sym_list ?
            "Associating node %d to group %d, weight %d (symmetry trial)." :
            "Associating node %d to group %d, weight %d.";
        ExtErrLog.add_log(ExtLogAssoc, msg, jlast, grp, mx);
    }
    if (gd_sym_list)
        gd_sym_list->new_grp_assoc(grp, jlast);
    return (true);
}


namespace {
    // Return the node number if the device has at least one contact
    // with an identified node, otherwise -1.
    //
    int check_contacts(cGroupDesc  *gd, const sDevInst *di)
    {
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
            if (!ci->cont_ok())
                continue;
            sGroup *g = gd->group_for(ci->group());
            if (!g)
                continue;  // shouldn't happen

            if (g->node() >= 0)
                return (g->node());
        }
        return (-1);
    }


    double factor(const cGroupDesc *gd, const sDevInst *di, bool brksym)
    {
        int ccnt = 0, acnt = 0;
        bool has_glob = false;
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
            if (!ci->cont_ok())
                continue;
            ccnt++;
            sGroup *g = gd->group_for(ci->group());
            if (!g)
                continue;
            if (g->node() >= 0)
                acnt++;
            if (!ci->desc()->is_bulk() && g->global())
                has_glob = true;
        }
        if (!ccnt)
            return (0.0);  // nonsense, bogus device

        // If the device is connected to a global node, knock it down
        // a level when doing symmetry trials.  This biases toward
        // devices connected only to regular nodes, which are better
        // defined.  In particular, in a MOS totem pole, the device
        // connected to the gate output will be symmetry matched
        // before the device connected to the rail.  Consider
        // parallel-connercted vectored NAND gates, with two such
        // having the lower input tied together.  One can see that all
        // of the NMOS transistors with grounded source are
        // equivalent, and can easily be symmetry-matched with a
        // transistor from the "wrong" gate, which will probably cause
        // association to go to its loop limit and ultimately fail. 
        // Forcing association of the other NMOS first prevents this.

        if (brksym && has_glob && acnt > 1)
            acnt -= 1;
        return (((double)acnt)/ccnt); 
    }
}


// Attempt to identify the electrical dual of the physical device
// given, from the list in edevs.  Return true if association
// established.
//
bool
cGroupDesc::ident_dev(sDevList *dv, int level, bool brksym) THROW_XIrt
{
    bool new_assoc = false;
    if (!dv->edevs())
        return (false);
    bool pass2 = false;
    sDevComp comp;

    bool justone = (!dv->edevs()->next() && !dv->prefixes()->next() &&
        !dv->prefixes()->devs()->next());
    if (justone) {
        if (!dv->prefixes()->devs()->dual()) {
            comp.set(dv->prefixes()->devs());
            find_match(dv, comp, brksym);
            return (true);
        }
        return (false);
    }

again:
    int ndevs = 0;
    int nnc = 0;
    for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
        if (!p->devs())
            continue;

        for (sDevInst *di = p->devs(); di; di = di->next()) {
            if (di->dual())
                // Already have dual.
                continue;
            ndevs++;
            if (!comp.set(di)) {
                ExtErrLog.add_err("In %s, error while associating:\n%s",
                    Tstring(gd_celldesc->cellname()), Errs()->get_error());
                continue;
            }

            if (!pass2) {
#ifdef USE_LEVELS
                if (level >= 0 && level < NUM_LEVELS) {
                    double f = factor(this, di, brksym);
                    double A = 1.0/NUM_LEVELS;
                    int d = NUM_LEVELS - level;
                    if (f <= (d - 1)*A || f > d*A)
                        continue;
                }
#endif
                if (check_contacts(this, di) < 0) {
                    ExtErrLog.add_log(ExtLogAssocV,
                        "dev %s %d, no identified node, skipping",
                        di->desc()->name(), di->index());
                    nnc++;
                    continue;
                }
            }

            // Find the device with the largest comparison score.  If
            // duplicates, pass unless brksym is true or the
            // duplicates are connected in parallel, in which case we
            // break the symmetry and choose one.

            set_no_cont_brksym(pass2);
            bool found = find_match(dv, comp, brksym);
            set_no_cont_brksym(false);
            if (found) {
                new_assoc = true;
                if (brksym)
                    return (true);
            }
        }
    }
    if (ndevs && nnc == ndevs && !pass2 && brksym && level <= 0) {
        // None of the devices of this type have a connection to a
        // group that has been associated.  Run again with this
        // requirement removed.  This allows the symmetry trials to
        // resolve an initial device association.

        pass2 = true;
        goto again;
    }
    return (new_assoc);
}


// Try to find the match for the physical device set into comp from
// among the electrical devices in dv.
// 
// True is returned if a new device association is made.
//
bool
cGroupDesc::find_match(sDevList *dv, sDevComp &comp, bool brksym) THROW_XIrt
{
    if (comp.nogood())
        return (false);

    sDevInst *di = comp.pdev();
    int mx = -1;
    sEinstList *dp = 0;
    sSymCll *eposs = 0;
    bool justone = (!dv->edevs()->next() && !dv->prefixes()->next() &&
        !dv->prefixes()->devs()->next());
    if (justone) {
        mx = 1000;
        di = dv->prefixes()->devs();
        dp = dv->edevs();
    }
    else {
        try {
            int px = -1;
            for (sEinstList *c = dv->edevs(); c;  c = c->next()) {
                if (c->dual_dev())
                    continue;
                comp.set(c);
                int n = comp.score(this);
                if (n > mx) {
                    px = -1;
                    mx = n;
                    dp = c;
                    if (brksym) {
                        sSymCll::destroy(eposs);
                        eposs = 0;
                    }
                    continue;
                }
                if (n <= 0 || n != mx || !dp)
                    continue;

                // Here, we've got a match to our best comparison score
                // from terminal comparisons.  Have a look at the
                // parameters.

                int p1 = ep_param_comp(di, dp);
                int p2 = ep_param_comp(di, c);
                if (p1 > 0 && p1 > p2)
                    continue;
                if (p2 > 0 && p2 > p1) {
                    if (p2 > px) {
                        // Found a new best, based on parameter
                        // comparison.

                        px = p2;
                        dp = c;
                        if (brksym) {
                            sSymCll::destroy(eposs);
                            eposs = 0;
                        }
                        continue;
                    }
                    if (p2 < px)
                        continue;
                }

                // Here, the devices are still matching, deal with it.

                if (brksym)
                    eposs = new sSymCll(c, eposs);
                else if (!comp.is_parallel(dp) && !comp.is_mos_tpeq(this, dp))
                    dp = 0;
            }

            if (!dp) {
                if (ExtErrLog.log_associating() && ExtErrLog.verbose()) {
                    sLstr lstr;
                    sDevContactInst *ci = di->contacts();
                    for ( ; ci; ci = ci->next()) {
                        lstr.add_c(' ');
                        lstr.add(Tstring(ci->desc()->name()));
                        lstr.add_c(' ');
                        lstr.add_i(ci->group());
                    }
                    ExtErrLog.add_log(ExtLogAssocV, "dev %s %d %d->0 %s",
                        di->desc()->name(), di->index(), mx, lstr.string());
                }
                return (false);
            }
            if (ExtErrLog.log_associating()) {
                char *instname =
                    dp->cdesc()->getElecInstName(dp->cdesc_index());
                ExtErrLog.add_log(ExtLogAssocV, "dev %s %d %d %s",
                    di->desc()->name(), di->index(), mx, instname);
                delete [] instname;
            }

            // This will permute the contact names if necessary.
            ep_param_comp(di, dp);
        }
        catch (XIrt) {
            sSymCll::destroy(eposs);
            throw;
        }
    }

    // Don't associate unless the score is reasonable.
    if (mx < CMP_SCALE/2) {

        sSymCll::destroy(eposs);
        // We can never associate the present device, so might as well
        // skip permuting if this is not a symmetry trial.

        if (!gd_sym_list) {
            set_skip_permutes(true);
            ExtErrLog.add_log(ExtLogAssoc,
                "device %s %d is not associable, skipping "
                "symmetry trials.", di->desc()->name(), di->index());
        }
        return (false);
    }

    if (eposs) {
        gd_sym_list = new sSymBrk(di, eposs, gd_sym_list);
        ExtErrLog.add_log(ExtLogAssoc, "Starting new symmetry trial.");
    }

    // Assign the duality.
    di->set_dual(dp);
    dp->set_dual(di);
    if (ExtErrLog.log_associating()) {
        char *instname = di->dual()->instance_name();
        const char *msg = gd_sym_list ?
            "Found dual device for %s %d: %s, weight %d (symmetry trial)." :
            "Found dual device for %s %d: %s, weight %d.";
        ExtErrLog.add_log(ExtLogAssoc, msg, di->desc()->name(),
            di->index(), instname, mx);
        delete [] instname;
    }
    if (gd_sym_list)
        gd_sym_list->new_dev_assoc(di, dp);

    // Now see if we can associate the groups connected to the
    // newly-associated device.
    //
    comp.set(dp);
    comp.associate(this);

    if (ExtErrLog.log_associating() && ExtErrLog.verbose()) {
        for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
            int g = ci->group();
            int found = false;
            if (g >= 0) {
                sDevContactList *dc = gd_groups[g].device_contacts();
                for ( ; dc; dc = dc->next()) {
                    if (dc->contact() == ci) {
                        found = true;
                        break;
                    }
                }
            }
            int n = g >= 0 ? gd_groups[g].node() : -1;
            int ng = n >= 0 ? group_of_node(n) : -1;
            ExtErrLog.add_log(ExtLogAssoc, 
                "  %s g=%d n=%d ng=%d found=%d",
                ci->desc()->name(), g, n, ng, found);
        }
    }
    return (true);
}


namespace {
    // Return true if the subcircuit has at least one contact with an
    // identified node, or if there are no contacts.
    //
    bool check_contacts(cGroupDesc *pgd, const sSubcInst *s)
    {
        CDs *sd = s->cdesc()->masterCell();
        cGroupDesc *mgd = sd ? sd->groups() : 0;
        for (sSubcContactInst *ci = s->contacts(); ci; ci = ci->next()) {

            sGroup *g = pgd->group_for(ci->parent_group());
            if (!g)
                continue;  // shouldn't happen


            // If the group has a node, we're good.
            if (g->node() >= 0)
                return (true);

            // Otherwise, if the contact is to metal only or the
            // subcircuit net is global, ignore it.
            if (mgd) {
                g = mgd->group_for(ci->subc_group());
                if (g && (g->global() || g->unas_wire_only()))
                    continue;
            }

            // The contact requires eventual node association.
            return (false);
        }
        // No applicable contacts, we're good.
        return (true);
    }


    double factor(const cGroupDesc *gd, const sSubcInst *s)
    {
        int ccnt = 0, acnt = 0;
        CDs *sd = s->cdesc()->masterCell();
        cGroupDesc *mgd = sd ? sd->groups() : 0;
        for (sSubcContactInst *ci = s->contacts(); ci; ci = ci->next()) {
            if (mgd) {
                sGroup *g = mgd->group_for(ci->subc_group());
                if (g && (g->global() || g->unas_wire_only()))
                    continue;
            }
            ccnt++;
            sGroup *g = gd->group_for(ci->parent_group());
            if (g && g->node() >= 0)
                acnt++;
        }
        if (!ccnt)
            return (1.0);
        return (((double)acnt)/ccnt);
    }
}


// Attempt to identify the electrical dual of the physical subcircuit
// given, from the list in esubs.  Return true if a new association is
// established.
//
bool
cGroupDesc::ident_subckt(sSubcList *scl, int level, bool brksym, bool perm_fix)
{
    bool new_assoc = false;
    sEinstList *esubs = scl->esubs();
    if (!esubs || !scl->subs())
        return (false);
    bool pass2 = false;

again:
    int nsubs = 0;
    int nnc = 0;
    bool singleton = (!scl->subs()->next() && !esubs->next());
    for (sSubcInst *s = scl->subs(); s; s = s->next()) {
        if (s->dual())
            // Already have dual.
            continue;
        nsubs++;

        if (!singleton && !pass2) {
            // See if the subcircuit has at least one contact with an
            // identified node.  If not, and the subcircuit has a
            // contact that could have an identified node, we will
            // pass.
            //
            if (!check_contacts(this, s)) {
                nnc++;
                char *iname = s->instance_name();
                ExtErrLog.add_log(ExtLogAssocV, "sub %s (no node)", iname);
                delete [] iname;
                continue;
            }

#ifdef USE_LEVELS
            if (!perm_fix && level >= 0 && level < NUM_LEVELS) {
                double f = factor(this, s);
                double A = 1.0/NUM_LEVELS;
                int d = NUM_LEVELS - level;
                if (f <= (d - 1)*A || f > d*A)
                    continue;
            }
#endif
        }

        set_no_cont_brksym(pass2);
        bool found = find_match(scl, s, brksym, perm_fix);
        set_no_cont_brksym(false);
        if (found) {
            new_assoc = true;
            if (brksym)
                break;
        }
    }
    if (nsubs && nnc == nsubs && !pass2 && brksym && level <= 0) {
        // None of the subcircuits of this type have a connection to a
        // group that has been associated.  Run again with this
        // requirement removed.  This allows the symmetry trials to
        // resolve an initial subcircuit association.

        pass2 = true;
        goto again;
    }
    return (new_assoc);
}


// Find the electrical subcircuit with the largest comparison score. 
// If duplicates, pass unless brksym is true or the duplicates are
// connected in parallel, in which case we break the symmetry and
// choose one.  True is returned if a new subcircuit association is
// made.
//
bool
cGroupDesc::find_match(sSubcList *sl, sSubcInst *si, bool brksym,
    bool perm_fix)
{
    int mx = -1;
    sEinstList *dp = 0;
    sSymCll *eposs = 0;
    if (!si->next() && !sl->esubs()->next()) {
        // Only one possibility...

        mx = CMP_SCALE;
        dp = sl->esubs();
    }
    else {
        for (sEinstList *c = sl->esubs(); c; c = c->next()) {
            if (c->dual_subc())
                continue;
            int n = ep_subc_comp(si, c, perm_fix);
            if (n > mx) {
                mx = n;
                dp = c;
                if (brksym) {
                    sSymCll::destroy(eposs);
                    eposs = 0;
                }
            }
            else if (n > 0 && n == mx && dp) {
                if (brksym)
                    eposs = new sSymCll(c, eposs);
                else if (!dp->is_parallel(c))
                    dp = 0;
            }
        }
    }
    if (!dp) {
        char *iname = si->instance_name();
        ExtErrLog.add_log(ExtLogAssocV, "sub %s 0", iname);
        delete [] iname;
        return (false);
    }
    if (ExtErrLog.log_associating()) {
        char *instname = dp->cdesc()->getElecInstName(dp->cdesc_index());
        char *iname = si->instance_name();
        ExtErrLog.add_log(ExtLogAssocV, "sub %s %d %s", iname, mx, instname);
        delete [] iname;
        delete [] instname;
    }

    // Don't associate unless the score is reasonable.
    if (mx < CMP_SCALE/2) {
        sSymCll::destroy(eposs);

        // We can never associate the present subcell, so might as
        // well skip permuting if this is not a symmetry trial.

        if (!gd_sym_list) {
            set_skip_permutes(true);
            char *iname = si->instance_name();
            ExtErrLog.add_log(ExtLogAssoc,
                "subckt %s is not associable, skipping symmetry trials.",
                iname);
            delete [] iname;
        }
        return (false);
    }

    if (eposs) {
        gd_sym_list = new sSymBrk(si, eposs, gd_sym_list);
        ExtErrLog.add_log(ExtLogAssoc, "Starting new symmetry trial.");
    }

    // Assign the duality.
    si->set_dual(dp);
    dp->set_dual(si);
    if (ExtErrLog.log_associating()) {
        char *instname = si->dual()->instance_name();
        const char *msg = gd_sym_list ?
            "Found dual subckt for %s: %s, weight %d (symmetry trial)." :
            "Found dual subckt for %s: %s, weight %d.";
        char *iname = si->instance_name();
        ExtErrLog.add_log(ExtLogAssoc, msg, iname, instname, mx);
        delete [] iname;
        delete [] instname;
    }
    if (gd_sym_list)
        gd_sym_list->new_subc_assoc(si, dp);

    const char *msg_g = gd_sym_list ?
        "Assigning node %d to group %d (symmetry trial)." :
        "Assigning node %d to group %d.";

    // Now see if we can associate the groups connected to the
    // newly-associated subcircuit.
    //
    for (sSubcContactInst *ci = si->contacts(); ci; ci = ci->next()) {
        int grp = ci->parent_group();
        if (grp < 0 || grp >= gd_asize)
            continue;
        if (gd_groups[grp].node() >= 0)
            continue;
        int node = ci->node(dp);
        if (node >= 0) {
            int gchk = group_of_node(node);
            if (gchk < 0 || gchk == grp) {
                gd_groups[grp].set_node(node);
                gd_etlist->set_group(node, grp);
                ExtErrLog.add_log(ExtLogAssoc, msg_g, node, grp);
                if (gd_sym_list)
                    gd_sym_list->new_grp_assoc(grp, node);
            }
        }
    }
    if (ExtErrLog.log_associating() && ExtErrLog.verbose()) {
        for (sSubcContactInst *ci = si->contacts(); ci; ci = ci->next()) {
            int g = ci->parent_group();
            bool found = false;
            if (g >= 0) {
                for (sSubcContactList *dc = gd_groups[g].subc_contacts();
                        dc; dc = dc->next()) {
                    if (dc->contact() == ci) {
                        found = true;
                        break;
                    }
                }
            }
            int n = g >= 0 ? gd_groups[g].node() : -1;
            int ng = n >= 0 ? group_of_node(n) : -1;
            ExtErrLog.add_log(ExtLogAssoc, "  g=%d n=%d ng=%d found=%d",
                g, n, ng, found);
        }
    }
    return (true);
}


// Fill in the node property pointer in the contacts.
//
void
cGroupDesc::ident_term_nodes(sDevInst *di)
{
    if (!di->dual())
        return;
    bool didp = false;
    for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
        if (ci->dev()->desc()->is_permute(ci->desc()->name())) {
            if (!didp) {
                sDevContactInst *ci1;
                for (ci1 = ci->next(); ci1; ci1 = ci1->next())
                    if (ci1->dev()->desc()->is_permute(ci1->desc()->name()))
                        break;
                if (ci1) {
                    CDp_cnode *pn = ci->node_prpty();
                    CDp_cnode *pn1 = ci1->node_prpty();
                    if (pn && pn1) {
                        int node1 = pn->enode();
                        int node2 = pn1->enode();
                        if (node1 >= 0 && node2 >= 0) {
                            int x1 = ep_grp_comp(ci->group(), node1);
                            int x2 = ep_grp_comp(ci->group(), node2);
                            int y1 = ep_grp_comp(ci1->group(), node2);
                            int y2 = ep_grp_comp(ci1->group(), node1);
                            if (x1 + y1 >= x2 + y2) {
                                // note the tie-breaking done here
                                ci->set_pnode(pn);
                                ci1->set_pnode(pn1);
                            }
                            else {
                                ci->set_pnode(pn1);
                                ci1->set_pnode(pn);
                            }
                            // set terminal marker location and layer
                            CDs *tsd =
                                CDcdb()->findCell(gd_celldesc->cellname(),
                                Electrical);
                            if (tsd) {
                                ci->set_term_loc(tsd, ci->desc()->ldesc());
                                ci1->set_term_loc(tsd, ci1->desc()->ldesc());
                            }
                        }
                    }
                }
                didp = true;
            }
        }
        else {
            ci->set_pnode(ci->node_prpty());

            // set terminal marker location and layer
            CDs *tsd = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
            if (tsd)
                ci->set_term_loc(tsd, ci->desc()->ldesc());
        }
    }
}



// Actually solve for the duals by iterating through the lists of
// groups, subcircuits, and devices and calling the ident functions.
// Return a count of unresolved references.
//
int
cGroupDesc::solve_duals() THROW_XIrt
{
    int psize = nextindex();

    set_allow_errs(false);
    if (first_pass()) {
        if (has_net_or_terms(0) && !conts_of_node(0)) {
            ExtErrLog.add_log(ExtLogAssoc,
                "Grounding inconsistency, group 0 nonempty, net 0 empty.");
        }
        else if (!has_net_or_terms(0) && conts_of_node(0)) {
            ExtErrLog.add_log(ExtLogAssoc,
                "Grounding inconsistency, group 0 empty, net 0 nonempty.");
        }
        else if (has_net_or_terms(0)) {
            ExtErrLog.add_log(ExtLogAssoc,
                "Associating ground group 0 to net 0.");
            gd_groups[0].set_node(0);
            gd_etlist->set_group(0, 0);
        }

        for (int i = 1; i < psize; i++) {
            if (gd_groups[i].termlist()) {
                CDp_snode *ps = gd_groups[i].termlist()->term()->node_prpty();
                if (!ps)
                    continue;
                int node = ps->enode();
                gd_groups[i].set_node(node);
                gd_etlist->set_group(node, i);
                ExtErrLog.add_log(ExtLogAssoc,
                    "Associated node %d to group %d from terminal assignment.",
                    node, i);

                // if more than one terminal, check consistency
                CDpin *pp = gd_groups[i].termlist(), *pn;
                for (CDpin *p = pp->next(); p; p = pn) {
                    pn = p->next();
                    ps = p->term()->node_prpty();
                    if (!ps)
                        continue;
                    if (ps->enode() != node) {
                        ExtErrLog.add_log(ExtLogAssoc,
                            "Terminal %s was initially assigned to group %d\n"
                            "  inconsistent terminal assignment ignored.",
                            p->term()->name(), i);
                        pp->set_next(pn);
                        p->term()->set_ref(0);
                        delete p;
                        continue;
                    }
                    pp = p;
                }
            }
        }
    }

    set_skip_permutes(first_pass() || CDvdb()->getVariable(VA_NoPermute));

    sSymBrk *saved = 0;
    int last = -1, lastlast = -1;
    int check_count = 0;
    int loop_count = 0;
    int max_iters = 0;
    unsigned long check_time = 0;
    try {
        for (;;) {
            if (loop_count > gd_loop_max) {
                sSymBrk::destroy(gd_sym_list);
                gd_sym_list = 0;
                break;
            }
            loop_count++;
            last = -1;
            int iter_count = 0;
            for (;;) {

                if (Timer()->check_interval(check_time)) {
                    if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wdraw())
                        dspPkgIf()->CheckForInterrupt();
                    if (XM()->ConfirmAbort())
                        throw (XIintr);
                }

                iter_count++;
                if (iter_count > max_iters)
                    max_iters = iter_count;

                int gcnt = 0;
                for (int i = 1; i < psize; i++) {
                    if (has_terms(i) && gd_groups[i].node() < 0) {
                        if (!check_global(i) && !check_unas_wire_only(i))
                            gcnt++;
                    }
                }
                int dcnt = 0;
                for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
                    for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                        for (sDevInst *di = p->devs(); di; di = di->next())
                            if (!di->dual())
                                dcnt++;
                    }
                }
                int scnt = 0;
                for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
                    for (sSubcInst *s = sl->subs(); s; s = s->next())
                        if (!s->dual())
                            scnt++;
                }
                ExtErrLog.add_log(ExtLogAssoc,
                    "loop %d, iteration %d: unresolved groups %d, "
                    "devs %d, subckts %d.",
                    loop_count, iter_count, gcnt, dcnt, scnt);

                set_grp_break(!dcnt && !scnt);

                int ecnt = gcnt + dcnt + scnt;
                if (EX()->isVerbosePromptline()) {
                    PL()->ShowPromptV("Loop %d, Iteration %d: unresolved %d",
                        loop_count, iter_count, ecnt);
                }
                if (!ecnt) {
                    last = ecnt;
                    break;
                }
                if (ecnt == last)
                    break;
                last = ecnt;

                if (iter_count > gd_iter_max)
                    break;

                int tmpsize = nextindex();  // this can change?
                for (int i = 1; i < tmpsize; i++)
                    ident_node(i);

#ifdef USE_LEVELS
                if (!dcnt && !scnt)
                    continue;
                bool didone = false;
                for (int lev = 0; lev < NUM_LEVELS; lev++) {
                    if (dcnt) {
                        for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
                            if (ident_dev(dv, lev))
                                didone = true;
                        }
                    }
                    if (scnt) {
                        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
                            if (ident_subckt(sl, lev))
                                didone = true;
                        }
                    }
                    if (didone)
                        break;
                }
#else
                if (dcnt) {
                    for (sDevList *dv = gd_devices; dv; dv = dv->next())
                        ident_dev(dv, -1);
                }
                if (scnt) {
                    for (sSubcList *sl = gd_subckts; sl; sl = sl->next())
                        ident_subckt(sl, -1);
                }
#endif
            }

            // Break symmetries,
            if (last) {
                if (last < lastlast) {
                    sSymBrk::destroy(saved);
                    saved = sSymBrk::dup(gd_sym_list);
                }
                lastlast = last;

                if (!skip_permutes() && break_symmetry())
                    continue;

                sSymBrk *sv = saved;
                while (sv) {
                    for (sSymGrp *sg = sv->grp_assoc(); sg;
                            sg = sg->next()) {
                        // associate group
                        ExtErrLog.add_log(ExtLogAssoc,
                            "Reassociating group %d from node %d "
                            "(best symmetry trial).",
                            sg->group(), sg->node());
                        gd_groups[sg->group()].set_node(sg->node());
                        gd_etlist->set_group(sg->node(), sg->group());
                    }

                    for (sSymDev *sd = sv->dev_assoc(); sd;
                            sd = sd->next()) {
                        // associate device
                        ExtErrLog.add_log(ExtLogAssoc,
                            "Reassociating device %s %d "
                            "(best symmetry trial).",
                            sd->phys_dev()->desc()->name(),
                            sd->phys_dev()->index());

                        sd->phys_dev()->set_dual(sd->elec_dev());
                        sd->elec_dev()->set_dual(sd->phys_dev());
                    }

                    for (sSymSubc *su = sv->subc_assoc(); su;
                            su = su->next()) {
                        // associate subckt
                        char *iname = su->phys_subc()->instance_name();
                        ExtErrLog.add_log(ExtLogAssoc,
                            "Reassociating subckt %s (best symmetry trial).",
                            iname);
                        delete [] iname;
                        su->phys_subc()->set_dual(su->elec_subc());
                        su->elec_subc()->set_dual(su->phys_subc());
                    }
                    sv = sv->next();
                }
            }
            sSymBrk::destroy(saved);
            saved = 0;

            if (!first_pass()) {
                // We still have errors, so association will not be
                // successful.  Take another pass, relaxing the
                // requirement that there can be no connection errors to
                // associated subcircuits.  This can cause more of the
                // circuit to be associated, making it easier to find the
                // errors.

                if (last && !allow_errs()) {
                    set_allow_errs(true);
                    continue;
                }

                // Check the terminal lists for each group/node association.
                // If there are errors, unassociate the devices or subckts in
                // question and try associating again.  This is tried twice.
                if (check_count < 2) {
                    check_split();
                    bool changed = false;
                    for (int i = 1; i < psize; i++) {
                        if (check_associations(i))
                            changed = true;
                    }
                    if (changed) {
                        check_count++;
                        continue;
                    }
                }
            }

            sSymBrk::destroy(gd_sym_list);
            gd_sym_list = 0;
            break;
        }
    }
    catch (XIrt) {
        sSymBrk::destroy(gd_sym_list);
        gd_sym_list = 0;
        sSymBrk::destroy(saved);
        saved = 0;
        throw;
    }

    ExtErrLog.add_log(ExtLogAssoc,
        "Solving for duals complete, loops %d, max iters %d, errs %d.",
        loop_count, max_iters, last);

    return (last);
}


// Look at the unassociated groups.  If the terminals consistently
// indicate another group, assume that the net is split, and set the
// split_group entries.
//
void
cGroupDesc::check_split()
{
    int psize = nextindex();
    for (int i = 1; i < psize; i++) {
        sGroup &g = gd_groups[i];

        // Initialize the split_groups entry!
        g.set_split_group(-1);

        // Ignore associated groups here.
        if (g.node() >= 0)
            continue;

        int node = -1;
        for (sDevContactList *dc = g.device_contacts(); dc; dc = dc->next()) {
            int n = dc->contact()->node();
            if (n >= 0) {
                if (node < 0)
                    node = n;
                else if (n != node) {
                    node = -2;
                    break;
                }
            }
        }
        if (node == -2)
            continue;
        for (sSubcContactList *sc = g.subc_contacts(); sc; sc = sc->next()) {
            int n = sc->contact()->node();
            if (n >= 0) {
                if (node < 0)
                    node = n;
                else if (n != node) {
                    node = -2;
                    break;
                }
            }
        }
        if (node >= 0) {
            int gchk = group_of_node(node);
            if (gchk >= 0) {
                g.set_split_group(gchk);
                if (ExtErrLog.log_associating()) {
                    char buf[256];
                    sprintf(buf, 
                        "Unassociated group %d looks like split of group "
                        "%d (node %d).",
                        i, gchk, node);
                    ExtErrLog.add_log(ExtLogAssoc, buf);
                }
            }
        }
    }
}


// Obtain the table of global names found in the electrical hierarchy,
// if needed.  Check if the group with number g is global.  If so, set
// the global flag in the sGroup and return true.
//
bool
cGroupDesc::check_global(int grp)
{
    if (grp < 1 || grp >= gd_asize)
        return (false);
    sGroup &g = gd_groups[grp];

    // To be "global", the group must have the flag set and a net name.
    if (g.global() && g.netname())
        return (true);
    if (g.global_tested())
        return (false);
    g.set_global_tested(true);

    if (!gd_global_nametab) {
        // The table contains all global names found in the electrical
        // hierarchy, as a CDnetName tag.  This is our copy and will be
        // freed in the destructor.

        CDs *esd = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
        if (esd)
            gd_global_nametab = SCD()->tabGlobals(esd);
    }
    if (!gd_global_nametab)
        return (false);
    CDnetName nn = g.netname();
    if (nn) {
        if (SymTab::get(gd_global_nametab, (unsigned long)nn) == 0) {
            // Has a global name.
            g.set_global(true);
            return (true);
        }
        if (SCD()->isGlobalNetName(Tstring(nn))) {
            // The name is a global name, but we haven't seen it in
            // the schematic hierarchy.  If the name ends with '!' it
            // is global, and the function call adds it to the table.

            g.set_global(true);
            return (true);
        }
        return (false);
    }

    // Test the connected subcircuit terminals, if any are global,
    // inherit the globalness.

    for (sSubcContactList *s = g.subc_contacts(); s; s = s->next()) {
        CDs *sd = s->contact()->subc()->cdesc()->masterCell(true);
        cGroupDesc *gd = sd->groups();
        if (!gd)
            continue;

        // ignore terminals of ignored subcircuits
        if (gd->test_nets_only())
            continue;

        sGroup *gp = gd->group_for(s->contact()->subc_group());
        if (!gp)
            continue;
        if (gp->global() && gp->netname()) {
            g.set_global(true);
            g.set_netname(gp->netname(), sGroup::NameFromTerm);
            return (true);
        }
        if (SymTab::get(gd_global_nametab, (unsigned long)gp->netname()) == 0) {
            g.set_global(true);
            g.set_netname(gp->netname(), sGroup::NameFromTerm);
            return (true);
        }
        if (gp->node() >= 0) {
            CDs *esd = CDcdb()->findCell(sd->cellname(), Electrical);
            if (esd) {
                CDp_snode *ps = (CDp_snode*)esd->prpty(P_NODE);
                for ( ; ps; ps = ps->next()) {
                    if (ps->enode() == gp->node()) {
                        if (SymTab::get(gd_global_nametab,
                                (unsigned long)ps->get_term_name()) == 0) {
                            g.set_global(true);
                            g.set_netname(ps->get_term_name(),
                                sGroup::NameFromTerm);
                            return (true);
                        }
                        break;
                    }
                }
            }
        }
    }
    return (false);
}


// Return true if the group is unassociated, has no device contacts,
// and connects only to similar contacts in subcircuits if there are
// subcircuit connections.  Such groups should be ignored in LVS.
//
bool
cGroupDesc::check_unas_wire_only(int grp)
{
    if (grp < 1 || grp >= gd_asize)
        return (true);
    sGroup &g = gd_groups[grp];
    if (g.unas_wire_only())
        return (true);
    if (g.unas_wire_only_tested())
        return (false);
    g.set_unas_wire_only_tested(true);

    if (g.device_contacts())
        return (false);
    if (g.node() >= 0)
        return (false);

    for (sSubcContactList *s = g.subc_contacts(); s; s = s->next()) {
        CDs *sd = s->contact()->subc()->cdesc()->masterCell(true);
        cGroupDesc *gd = sd->groups();
        if (!gd)
            continue;
        if (!gd->check_unas_wire_only(s->contact()->subc_group()))
            return (false);
    }
    g.set_unas_wire_only(true);
    return (true);
}


// Check the terminal references.  If errors are found, unassociate the
// devices/subcircuits containing error terminals and return true.
//
bool
cGroupDesc::check_associations(int grp)
{
    sGroup *g = group_for(grp);
    if (!g || g->node() < 0 || !g->has_terms())
        return (false);

    bool retval = false;
    int tcnt = 0;
    for (CDcont *t = conts_of_node(g->node()); t; t = t->next())
        tcnt++;
    CDcterm **terms = new CDcterm*[tcnt];

    // List and check device contacts.
    tcnt = 0;
    for (CDcont *t = conts_of_node(g->node()); t; t = t->next()) {
        CDc *cd = t->term()->instance();
        if (!cd || !cd->isDevice())
            continue;
        terms[tcnt++] = t->term();
    }

    if (ExtErrLog.log_associating() && ExtErrLog.verbose()) {
        sLstr lstr;
        lstr.add("Checking group ");
        lstr.add_i(grp);
        lstr.add_c('\n');
        lstr.add("  Elec Dev:");
        for (int i = 0; i < tcnt; i++) {
            lstr.add_c(' ');
            lstr.add(Tstring(terms[i]->master_name()));
        }
        lstr.add("\n  Phys Dev:");
        for (sDevContactList *dc = g->device_contacts(); dc; dc = dc->next()) {
            lstr.add_c(' ');
            lstr.add(Tstring(dc->contact()->desc()->name()));
        }
        ExtErrLog.add_log(ExtLogAssoc, lstr.string());
    }
    for (sDevContactList *dc = g->device_contacts(); dc; dc = dc->next()) {
        sDevInst *di = dc->contact()->dev();
        bool found = false;
        for (int i = 0; i < tcnt; i++) {
            if (!terms[i])
                continue;
            if (di->dual() && di->dual()->cdesc() != terms[i]->instance())
                continue;
            if (di->desc()->name() != terms[i]->instance()->cellname())
                continue;
            if (terms[i]->master_name() == dc->contact()->desc()->name()) {
                found = true;
                terms[i] = 0;
                break;
            }
            else if (di->desc()->is_permute(dc->contact()->desc()->name())) {
                if (di->desc()->is_permute(terms[i]->master_name())) {
                    found = true;
                    terms[i] = 0;
                    break;
                }
            }
        }
        if (!found && di->dual()) {
            ExtErrLog.add_log(ExtLogAssoc,
                "Unassociating %s %d, %s not connected to node.",
                di->desc()->name(), di->index(),
                dc->contact()->desc()->name());
            retval = true;

            // unassociate device
            di->dual()->set_dual((sDevInst*)0);
            di->set_dual(0);

            // unassociate connected nodes
            for (sDevContactInst *ci = di->contacts(); ci; ci = ci->next()) {
                int grpnum = ci->group();
                if (grpnum < 0 || grpnum >= gd_asize)
                    continue;
                int node = gd_groups[grpnum].node();
                if (node >= 0) {
                    gd_groups[grpnum].set_node(-1);
                    gd_etlist->set_group(node, -1);
                    ExtErrLog.add_log(ExtLogAssoc,
                        "Unassociating group %d from node %d.",
                        grpnum, node);
                }
            }
        }
    }

    // List and check subcircuit contacts.
    tcnt = 0;
    for (CDcont *t = conts_of_node(g->node()); t; t = t->next()) {
        CDc *cd = t->term()->instance();
        if (!cd || cd->isDevice())
            continue;
        terms[tcnt++] = t->term();
    }
    if (ExtErrLog.log_associating() && ExtErrLog.verbose()) {
        char buf[256];
        sLstr lstr;
        lstr.add("  Elec Subc:");
        for (int i = 0; i < tcnt; i++) {
            sprintf(buf, " %s:%s:%d",
                Tstring(terms[i]->instance()->cellname()),
                TstringNN(terms[i]->master_name()), terms[i]->master_group());
            lstr.add(buf);
        }
        lstr.add("\n  Phys Subc: ");
        for (sSubcContactList *sc = g->subc_contacts(); sc; sc = sc->next()) {
            sSubcContactInst *ci = sc->contact();
            sEinstList *el = ci->subc()->dual();
            sprintf(buf, " %s:%d:%d",
                el ? Tstring(el->cdesc()->cellname()) : "",
                ci->parent_group(), ci->subc_group());
            lstr.add(buf);
        }
        ExtErrLog.add_log(ExtLogAssoc, lstr.string());
    }
    for (sSubcContactList *sc = g->subc_contacts(); sc; sc = sc->next()) {
        sSubcContactInst *ci = sc->contact();
        sSubcInst *subc = ci->subc();
        CDs *sd = subc->cdesc()->masterCell(true);
        cGroupDesc *gd = sd->groups();
        if (!gd)
            continue;

        // ignore terminals of ignored subcircuits
        if (gd->test_nets_only())
            continue;

        bool found = false;
        for (int i = 0; i < tcnt; i++) {
            if (!terms[i])
                continue;
            if (subc->dual() && subc->dual()->cdesc() != terms[i]->instance())
                continue;
            if (sd->cellname() != terms[i]->instance()->cellname())
                continue;

            int subg = terms[i]->master_group();
            // This will fail unless we make sure that the vgroup is
            // always set in virtual terminals, even when not placed.

            if (subg < 0) {
                // Terminal not placed, must be a non-connected net,
                // or perhaps a global.

                if (ci->is_wire_only() || ci->is_global()) {
                    found = true;
                    terms[i] = 0;
                    break;
                }
            }
            else if (subg == ci->subc_group() || (subc->permutes() &&
                    subc->permutes()->is_equiv(subg, ci->subc_group()))) {
                found = true;
                terms[i] = 0;
                break;
            }
        }

        // If the contact is global, lack of a node is not an error.
        if (g->global() && g->netname())
            found = true;
        // If the contact is wire-only, lack of a node is not an error.
        if (ci->is_wire_only())
            found = true;

        if (!found && sc->contact()->subc()->dual()) {
            char *iname = subc->instance_name();
            ExtErrLog.add_log(ExtLogAssoc,
                "Unassociating %s, group %d not connected to node.",
                iname, ci->subc_group());
            delete [] iname;
            retval = true;

            // unassociate subcircuit
            subc->dual()->set_dual((sSubcInst*)0);
            subc->set_dual(0);

            // unassociate connected nodes
            for (sSubcContactInst *sci = subc->contacts(); sci;
                    sci = sci->next()) {
                int grpnum = sci->parent_group();
                if (grpnum < 0 || grpnum >= gd_asize)
                    continue;
                int node = gd_groups[grpnum].node();
                if (node >= 0) {
                    gd_groups[grpnum].set_node(-1);
                    gd_etlist->set_group(node, -1);
                    ExtErrLog.add_log(ExtLogAssoc,
                        "Unassociating group %d from node %d.",
                        grpnum, node);
                }
            }
        }
    }

    delete [] terms;
    return (retval);
}


// If there is a symmetry which has prevented convergence, make an
// arbitrary choice to break the symmetry, and return true.
//
bool
cGroupDesc::break_symmetry() THROW_XIrt
{
    // Return false if there is a mismatch in the number of
    // unassigned devices.
    int dnum = 0;
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        int pucnt = 0, ptcnt = 0;
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *s = p->devs(); s; s = s->next()) {
                ptcnt++;
                if (!s->dual())
                    pucnt++;
            }
        }
        dnum += pucnt;
        int eucnt = 0, etcnt = 0;
        for (sEinstList *c = dv->edevs(); c; c = c->next()) {
            etcnt++;
            if (!c->dual_dev())
                eucnt++;
        }
        if (eucnt != pucnt) {
            ExtErrLog.add_log(ExtLogAssoc,
                "Inconsistent unassociation count for device %s,\n"
                "  elec %d of %d, phys %d of %d, skipping symmetry trials.",
                Tstring(dv->prefixes()->devs()->desc()->name()),
                eucnt, etcnt, pucnt, ptcnt);
            return (false);
        }
    }

    // Return false if there is a mismatch in the number of
    // unassigned subckts.
    int snum = 0;
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        int pucnt = 0, ptcnt = 0;
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            ptcnt++;
            if (!s->dual())
                pucnt++;
        }
        snum += pucnt;
        int eucnt = 0, etcnt = 0;
        for (sEinstList *c = sl->esubs(); c; c = c->next()) {
            etcnt++;
            if (!c->dual_subc())
                eucnt++;
        }
        if (pucnt != eucnt) {
            ExtErrLog.add_log(ExtLogAssoc,
                "Inconsistent unassociation count for subcircuit %s,\n"
                "  elec %d of %d, phys %d of %d, skipping symmetry trials.",
                Tstring(sl->subs()->cdesc()->cellname()),
                eucnt, etcnt, pucnt, ptcnt);
            return (false);
        }
    }

#ifdef USE_LEVELS
    for (int lev = 0; lev < NUM_LEVELS; lev++) {
        if (dnum) {
            // break symmetry for devices
            try {
                for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
                    if (ident_dev(dv, lev, true))
                        return (true);
                }
            }
            catch (XIrt) {
                throw;
            }
        }
        if (snum) {
            // break symmetry for subckts
            for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
                if (ident_subckt(sl, lev, true))
                    return (true);
            }
        }
    }
#else
    if (dnum) {
        // break symmetry for devices
        try {
            for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
                if (ident_dev(dv, -1, true))
                    return (true);
            }
        }
        catch (XIrt) {
            throw;
        }
    }
    if (snum) {
        // break symmetry for subckts
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            if (ident_subckt(sl, -1, true))
                return (true);
        }
    }
#endif

    while (gd_sym_list) {
        ExtErrLog.add_log(ExtLogAssoc, "Reverting from symmetry trial.");
        for (sSymGrp *sg = gd_sym_list->grp_assoc(); sg; sg = sg->next()) {
            // unassociate group
            ExtErrLog.add_log(ExtLogAssoc,
                "Unassociating group %d from node %d (symmetry trial).",
                sg->group(), sg->node());
            gd_groups[sg->group()].set_node(-1);
            gd_etlist->set_group(sg->node(), -1);
        }
        sSymGrp::destroy(gd_sym_list->grp_assoc());
        gd_sym_list->set_grp_assoc(0);

        for (sSymDev *sd = gd_sym_list->dev_assoc(); sd; sd = sd->next()) {
            // unassociate device
            ExtErrLog.add_log(ExtLogAssoc,
                "Unassociating device %s %d (symmetry trial).",
                sd->phys_dev()->desc()->name(), sd->phys_dev()->index());
            sd->phys_dev()->set_dual(0);
            sd->elec_dev()->set_dual((sDevInst*)0);
        }
        sSymDev::destroy(gd_sym_list->dev_assoc());
        gd_sym_list->set_dev_assoc(0);

        for (sSymSubc *su = gd_sym_list->subc_assoc(); su; su = su->next()) {
            // unassociate subckt
            char *iname = su->phys_subc()->instance_name();
            ExtErrLog.add_log(ExtLogAssoc,
                "Unassociating subckt %s (symmetry trial).", iname);
            delete [] iname;
            su->phys_subc()->set_dual(0);
            su->elec_subc()->set_dual((sSubcInst*)0);
        }
        sSymSubc::destroy(gd_sym_list->subc_assoc());
        gd_sym_list->set_subc_assoc(0);

        if (gd_sym_list->elec_insts()) {
            // try the next possibility
            if (gd_sym_list->device()) {
                sEinstList *dp = gd_sym_list->elec_insts()->inst_elem();
                sDevInst *di = gd_sym_list->device();

                sSymCll *cll = gd_sym_list->elec_insts();
                gd_sym_list->set_elec_insts(cll->next());
                delete cll;

                // Assign the duality.
                di->set_dual(dp);
                dp->set_dual(di);
                if (ExtErrLog.log_associating()) {
                    char *instname = di->dual()->instance_name();
                    ExtErrLog.add_log(ExtLogAssoc,
                    "Associating dual device for %s %d: %s (symmetry trial).",
                        di->desc()->name(), di->index(), instname);
                    delete [] instname;
                }
                if (gd_sym_list)
                    gd_sym_list->new_dev_assoc(di, dp);

                sDevComp comp;
                if (comp.set(di) && comp.set(dp))
                    comp.associate(this);
                else {
                    ExtErrLog.add_err("In %s, error while associating:\n%s",
                        Tstring(gd_celldesc->cellname()),
                        Errs()->get_error());
                }
            }
            else if (gd_sym_list->subckt()) {
                sEinstList *dp = gd_sym_list->elec_insts()->inst_elem();
                sSubcInst *s = gd_sym_list->subckt();

                sSymCll *cll = gd_sym_list->elec_insts();
                gd_sym_list->set_elec_insts(cll->next());
                delete cll;

                // Assign the duality.
                s->set_dual(dp);
                dp->set_dual(s);
                if (ExtErrLog.log_associating()) {
                    char *instname = s->dual()->instance_name();
                    char *iname = s->instance_name();
                    ExtErrLog.add_log(ExtLogAssoc,
                        "Associating dual subckt for %s: %s (symmetry trial).",
                        iname, instname);
                    delete [] iname;
                    delete [] instname;
                }
                if (gd_sym_list)
                    gd_sym_list->new_subc_assoc(s, dp);

                const char *msg_g =
                    "Assigning node %d to group %d (symmetry trial).";

                // Now see if we can associate the groups connected to the
                // newly-associated subcircuit.
                //
                for (sSubcContactInst *ci = s->contacts(); ci;
                        ci = ci->next()) {
                    int grp = ci->parent_group();
                    if (grp < 0 || grp >= gd_asize)
                        continue;
                    if (gd_groups[grp].node() >= 0)
                        continue;
                    int node = ci->node(dp);
                    if (node >= 0) {
                        int gchk = group_of_node(node);
                        if (gchk < 0 || gchk == grp) {
                            gd_groups[grp].set_node(node);
                            gd_etlist->set_group(node, grp);
                            ExtErrLog.add_log(ExtLogAssoc, msg_g, node, grp);
                            if (gd_sym_list)
                                gd_sym_list->new_grp_assoc(grp, node);
                        }
                    }
                }
            }
            return (true);
        }
        // no more candidates
        sSymBrk *sb = gd_sym_list;
        gd_sym_list = sb->next();
        delete sb;
    }
    return (false);
}
// End of cGroupDesc functions.


int
sSubcContactInst::node(const sEinstList *el) const
{
    sSubcInst *si = subc();
    if (!el)
        el = si->dual();
    if (!el)
        return (-1);
    CDs *sd = el->cdesc()->masterCell();
    if (!sd)
        return (-1);

    int index = -1;
    CDp_snode *ps = (CDp_snode*)sd->prpty(P_NODE);
    for ( ; ps; ps = ps->next()) {
        if (ps->cell_terminal() &&
                ps->cell_terminal()->group() == subc_group()) {
            index = ps->index();
            break;
        }
    }
    if (index < 0)
        return (-1);

    unsigned int vix = el->cdesc_index();
    if (vix > 0) {
        CDp_range *pr = (CDp_range*)el->cdesc()->prpty(P_RANGE);
        if (pr) {
            CDp_cnode *pc = pr->node(0, vix, index);
            if (pc)
                return (pc->enode());
        }
    }
    else {
        CDp_cnode *pc = (CDp_cnode*)el->cdesc()->prpty(P_NODE);
        for ( ; pc; pc = pc->next()) {
            if (pc->index() == (unsigned int)index)
                return (pc->enode());
        }
    }
    return (-1);
}
// End of sSubcContactInst functions.


// Zero the dual pointer and put the global contacts back into the
// main list, as they were initially.
//
void
sSubcInst::clear_duality()
{
    set_dual(0);
    sSubcContactInst *ci = sc_glob_conts;
    sc_glob_conts = 0;
    if (ci) {
        sSubcContactInst *cn = ci;
        while (cn->next())
            cn = cn->next();
        cn->set_next(sc_contacts);
        sc_contacts = ci;
    }
}


// Set the locations for the physical instance terminals.
//
void
sSubcInst::place_phys_terminals()
{
    if (!sc_dual)
        return;
    CDc *ecdesc = sc_dual->cdesc();
    if (!ecdesc)
        return;
    CDs *esdesc = ecdesc->masterCell(true);
    if (!esdesc)
        return;
    int ec_vecix = sc_dual->cdesc_index();
    CDc *pcdesc = sc_cdesc;

    CDp_snode **node_ary;
    unsigned int asz = esdesc->checkTerminals(&node_ary);
    if (!asz)
        return;

    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(pcdesc);
    CDap ap(pcdesc);
    stk.TTransMult(sc_ix*ap.dx, sc_iy*ap.dy);

    CDp_range *pr = (CDp_range*)ecdesc->prpty(P_RANGE);
    CDp_cnode *cpn0 = (CDp_cnode*)ecdesc->prpty(P_NODE);
    for ( ; cpn0; cpn0 = cpn0->next()) {

        CDp_cnode *cpn;
        if (ec_vecix > 0 && pr)
            cpn = pr->node(0, ec_vecix, cpn0->index());
        else
            cpn = cpn0;
        if (!cpn)
            continue;

        CDcterm *cterm = cpn->inst_terminal();
        if (!cterm)
            continue;
        unsigned int n = cpn->index();
        if (n < asz) {
            CDp_snode *spn = node_ary[n];
            if (spn && spn->cell_terminal()) {
                CDsterm *sterm = spn->cell_terminal();
                int x = sterm->lx();
                int y = sterm->ly();
                stk.TPoint(&x, &y);
                cterm->set_loc(x, y);

                // Set the vgroup to the group number.  This allows
                // the group() method to return a correct value, since
                // the t_oset is never set for instance terminals.
                //
                int node = cpn->enode();
                cGroupDesc *gd = pcdesc->parent()->groups();
                if (gd) {
                    int grp = gd->group_of_node(node);
                    if (grp >= 0)
                        cterm->set_v_group(grp);
                }

                // Set the layer field of the instance terminal to
                // the correct layer, so on-screen mark will show this.
                //
                cterm->set_layer(sterm->layer());
                cterm->set_uninit(false);
            }
        }
    }
    stk.TPop();
    delete [] node_ary;
}


// This detects and set up the contact permutation groups.  This must
// be done after calling post-extract on the WHOLE HIERARCHY, as the
// templates for low-level cells may be updated to account for virtual
// contacts only seen at high levels.
//
// At this point, it is required that 1) the contact group templates
// are static (final), and 2) the group numbering is static.
//
void
sSubcInst::pre_associate()
{
    CDs *sdesc = cdesc()->masterCell();
    if (!sdesc)
        return;

    // This shouldn't be set yet.
    sExtPermGrp<int>::destroy(sc_permutes);
    sc_permutes = 0;

    sSubcDesc *scd = EX()->findSubcircuit(sdesc);
    if (!scd)
        return;
    if (!scd->setup_permutes())
        return;
    unsigned int numgrps = scd->num_groups();
    if (!numgrps)
        return;
    sExtPermGrp<int> *pe = 0;
    for (unsigned int i = 0; i < numgrps; i++) {
        sExtPermGrp<int> *p = new sExtPermGrp<int>;
        unsigned int gsz = scd->group_size(i);
        p->set(scd->group_ary(i), gsz);
        for (unsigned int j = 0; j < gsz; j++) {
            int g = *p->get(j);
            for (sSubcContactInst *c = sc_contacts; c; c = c->next()) {
                if (c->subc_group() == g) {
#ifdef PERM_DEBUG
                    printf("adding permutable group %d\n", g);
#endif
                    p->set_target(c->subc_group_addr(), j);
                    break;
                }
            }
        }
        if (!sc_permutes)
            sc_permutes = pe = p;
        else {
            pe->set_next(p);
            pe = pe->next();
        }
    }
#ifdef PERM_DEBUG
    sExtPermGen<int> gen(sc_permutes);
    while (gen.next()) {
        printf("%d :", sc_permutes->state());
        for (sSubcContactInst *ci = sc_contacts; ci; ci = ci->next())
            printf(" %d", ci->subc_group());
        printf("\n");
    }
    printf("\n");
#endif
}


// This removes global nets from the contact lists and moves them to
// separate lists, which is more convenient for output.
//
void
sSubcInst::post_associate()
{
    CDs *sdesc = cdesc()->masterCell();
    if (!sdesc)
        return;

    // Reinitialize the subcircuit permutations, if any.
    for (sExtPermGrp<int> *p = sc_permutes; p; p = p->next()) {
        p->reset();
        for (unsigned int i = 0; i < p->num_permutes(); i++) {
            for (sSubcContactInst *ci = contacts(); ci; ci = ci->next()) {
                if (ci->subc_group() == *p->get(i)) {
                    p->set_target(ci->subc_group_addr(), i);
                    break;
                }
            }
        }
    }

    // We may have added virtual contacts.
    update_template();

    cGroupDesc *gd = sdesc->groups();
    if (!gd)
        return;
    sSubcContactInst *cp = 0, *cn, *ce = 0;
    for (sSubcContactInst *ci = contacts(); ci; ci = cn) {
        cn = ci->next();
        sGroup *g = gd->group_for(ci->subc_group());
        if (g && g->global() && g->netname()) {
            if (cp)
                cp->set_next(cn);
            else
                set_contacts(cn);
            if (!sc_glob_conts)
                sc_glob_conts = ce = ci;
            else {
                ce->set_next(ci);
                ce = ce->next();
            }
            ci->set_next(0);
            continue;
        }
        cp = ci;
    }
}
// End of sSubcInst functions.


void
sSubcDesc::find_and_set_permutes()
{
    cGroupDesc *gd = master()->groups();
    if (!gd)
        return;
    sPermGrpList *pgl = gd->check_permutes();
    if (!pgl)
        return;

    if (pgl && ExtErrLog.log_associating()) {
        sLstr lstr;
        lstr.add("Permutable contacts detected in ");
        lstr.add(Tstring(master()->cellname()));
        lstr.add_c(':');
        for (sPermGrpList *p = pgl; p; p = p->next()) {
            lstr.add_c(' ');
            if (p->type() == PGnand)
                lstr.add("NAND");
            else if (p->type() == PGnor)
                lstr.add("NOR");
            else
                lstr.add("TOPO");
            lstr.add_c('(');
            for (unsigned int i = 0; i < p->size(); i++) {
                lstr.add_c(' ');
                lstr.add_i(p->group(i));
            }
            lstr.add(" )");
        }
        ExtErrLog.add_log(ExtLogExt, lstr.string());
    }

    int num_perm_grps = 0;
    int num_perms = 0;
    for (sPermGrpList *p = pgl; p; p = p->next()) {
        num_perm_grps++;
        num_perms += p->size() + 1;
    }

    int *a = new int[num_contacts() + 2 + num_perm_grps + num_perms];
    int top = num_contacts() + 1;
    for (int i = 0; i < top; i++)
        a[i] = sd_array[i];
    int *b = a + top;
    *b++ = num_perm_grps;
    for (sPermGrpList *p = pgl; p; p = p->next()) {
        *b++ = p->size();
        *b++ = p->type();
        for (unsigned int i = 0; i < p->size(); i++)
            *b++ = p->group(i);
    }
    sPermGrpList::destroy(pgl);
    delete [] sd_array;
    sd_array = a;
#ifdef PERM_DEBUG
    {
        a = sd_array;
        int n = a[0];
        printf("%d:", n);
        for (int i = 0; i < n; i++)
            printf(" %d", a[i+1]);
        printf("\n");
        b = a + n + 1;
        n = *b;
        printf("%d\n", n);
        b++;
        for (int i = 0; i < n; i++) {
            printf("  %d %d:", group_size(i), group_type(i));
            int *z = group_ary(i);
            for (int j = 0; j < nn; j++)
                printf(" %d", z[j]);
            printf("\n");
        }
        printf("---\n");
    }
#endif
}
// End of sSubcDesc functions.

