
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
#include "ext_nets.h"
#include "sced.h"
#include "sced_param.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_hypertext.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"

/*========================================================================*
 *
 *  Functions for dumping LVS results from the extraction system.
 *
 *========================================================================*/


// Record a comparison between the physical and electrical cells,
// recursively.
//
LVSresult
cExt::lvs(FILE *fp, CDcbin *cbin, int depth)
{
    if (!cbin || !cbin->elec())
        return (LVSap);
    DSPpkg::self()->SetWorking(true);

    SymTab *tab = new SymTab(false, false);
    if (!associate(cbin->phys())) {
        delete tab;
        DSPpkg::self()->SetWorking(false);
        return (LVSerror);
    }

    // For evaluating electrical device parameters.
    cParamCx *pcx = new cParamCx(cbin->elec());
    setParamCx(pcx);

    cGroupDesc *gd = cbin->phys()->groups();
    if (gd)
        gd->set_top_level(true);
    LVSresult ret = lvs_recurse(fp, cbin, depth, tab);
    delete tab;
    if (gd)
        gd->set_top_level(false);

    setParamCx(0);
    delete pcx;

    DSPpkg::self()->SetWorking(false);
    return (ret);
}


LVSresult
cExt::lvs_recurse(FILE *fp, CDcbin *cbin, int depth, SymTab *tab)
{
    // update connectivity
    if (!cbin || !cbin->elec() || !cbin->phys())
        return (LVSap);
    CDs *esdesc = cbin->elec();
    CDs *psdesc = cbin->phys();
    tab->add((uintptr_t)esdesc, 0, false);
    fprintf(fp, "############  %s\n", XM()->IdString());
    fprintf(fp, "LVS comparison for cell %s.\n", Tstring(cbin->cellname()));
    cGroupDesc *gd = psdesc->groups();
    if (!gd || gd->isempty()) {
        fprintf(fp, "No physical nets found.\n");
        const char *fmt = "\nLVS of cell %s %s.\n\n";
        fprintf(fp, fmt, Tstring(cbin->cellname()), "FAILED");
        return (LVSfail);
    }

    if (!gd->top_level() && paramCx())
        paramCx()->push(esdesc, 0);
    LVSresult res = gd->print_lvs(fp);
    if (!gd->top_level() && paramCx())
        paramCx()->pop();

    if (depth > 0) {
        CDm_gen mgen(esdesc, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            if (!mdesc->hasInstances())
                continue;
            CDs *msdesc = mdesc->celldesc();
            if (!msdesc)
                continue;
            if (SymTab::get(tab, (uintptr_t)msdesc) != ST_NIL)
                continue;
            if (EX()->skipExtract(msdesc))
                continue;
            CDcbin tcbin(msdesc);
            if (!tcbin.phys())
                continue;

            // If all of the instances are flattened, do *not* recurse!
            bool doit = false;
            CDc_gen cgen(mdesc);
            for (CDc *cd = cgen.c_first(); cd; cd = cgen.c_next()) {
                if (EX()->shouldFlatten(cd, esdesc))
                    continue;
                doit = true;
                break;
            }
            if (doit) {
                LVSresult tres = lvs_recurse(fp, &tcbin, depth-1, tab);
                if (tres < res)
                    res = tres;
            }
        }
    }

    // Return value is never LVSerror.
    return (res);
}
// End of cExt functions.


namespace {
    // Count the number of contact points, subcircuit terminals, and
    // device terminals in the list.
    //
    void
    tcount(CDcont *t0, int *st, int *dt)
    {
        *st = 0;
        *dt = 0;
        for (CDcont *t = t0; t; t = t->next()) {
            if (!t->term()->instance())
                continue;
            CDs *sd = t->term()->instance()->masterCell(true);
            if (!sd)
                continue;
            if (sd->isDevice())
                // device
                (*dt)++;
            else
                // subcircuit
                (*st)++;
        }
    }
}


struct sLVSstat
{
    sLVSstat()
       {
            bad_terms = 0;
            bad_nets = 0;
            bad_conts = 0;
            poss_bad_conts = 0;
            uc_cnt = 0;
            val_cmp_errs = 0;
            val_amb_cnt = 0;

            gp_unassoc = 0;
            pd_unassoc = 0;
            ps_unassoc = 0;
            nt_unassoc = 0;
            ed_unassoc = 0;
            es_unassoc = 0;
            dc_unassoc = 0;
            sc_unassoc = 0;
            dc_assoc = 0;
            sc_assoc = 0;
            need_note1 = false;
       }

    int bad_terms;
    int bad_nets;
    int bad_conts;      // globally unresolved bulk contacts
    int poss_bad_conts; // locally unresolved bulk contacts
    int uc_cnt;
    int val_cmp_errs;
    int val_amb_cnt;

    int gp_unassoc;     // unassociated groups
    int pd_unassoc;     // unassociated physical devices
    int ps_unassoc;     // unassociated physical subcircuits
    int nt_unassoc;     // unassociated nets
    int ed_unassoc;     // unassociated electrical devices
    int es_unassoc;     // unassociated electrical subcircuits
    int dc_unassoc;     // associated physical device contacts to
                        // unassociated group
    int sc_unassoc;     // associated physical subcircuit contacts to
                        // unassociated group
    int dc_assoc;       // unassociated physical device contacts to
                        // associated group
    int sc_assoc;       // unassociated physical subcircuit contacts to
                        // associated group
    bool need_note1;
};


// Print the LVS result.
//
LVSresult
cGroupDesc::print_lvs(FILE *fp)
{
    sLVSstat lvs;
    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    int psize = nextindex();
    int esize = gd_etlist ? gd_etlist->size() : 0;

    if (top_level()) {
        sBcErr *bcerrs = check_stamping();
        if (bcerrs) {
            fprintf(fp,
        "\nIncorrectly resolved bulk device contacts found in hierarchy:\n\n");
            fprintf(fp, "%-12s %-18s %s\n", "Device", "Containing Cell",
                "Top Level Bounding Box");
            for (sBcErr *b = bcerrs; b; b = b->next()) {
                lvs.bad_conts++;
                fprintf(fp, "%-12s %-18s %.4f,%.4f %.4f,%.4f\n",
                    Tstring(b->devDesc()->name()),
                    Tstring(b->owner()->cellname()),
                    MICRONS(b->locBB()->left), MICRONS(b->locBB()->bottom),
                    MICRONS(b->locBB()->right), MICRONS(b->locBB()->top));
            }
            fprintf(fp, "\n");
            sBcErr::destroy(bcerrs);
        }
    }

    // Print table of group/node associations.
    fprintf(fp, "\nConductor group and electrical node mapping:\n\n");
    fprintf(fp, "  %-7s %-23s %-23s %-8s\n", "group", "node", "node", "group");
    char tbuf[64];
    for (int i = 1; i < psize || i < esize; i++) {
        char gr1[32];
        if (i < psize && has_net_or_terms(i))
            sprintf(gr1, "%d", i);
        else
            *gr1 = 0;

        char nname2[64];
        if (i < psize && gd_groups[i].node() >= 0) {
            const char *nn = SCD()->nodeName(esdesc, gd_groups[i].node());
            sprintf(nname2, "(%d) %s", gd_groups[i].node(), nn);
        }
        else {
            if (i < psize && has_net_or_terms(i)) {
                if (gd_groups[i].global() && gd_groups[i].netname())
                    strcpy(nname2, Tstring(gd_groups[i].netname()));
                else
                    strcpy(nname2, "---");
            }
            else
                *nname2 = 0;
        }

        char nname1[64];
        if (i < esize && node_active(i)) {
            const char *nn = SCD()->nodeName(esdesc, i);
            sprintf(nname1, "(%d) %s", i, nn);
        }
        else
            *nname1 = 0;

        char gr2[32];
        if (i < esize && group_of_node(i) >= 0)
            sprintf(gr2, "%d", group_of_node(i));
        else {
            if (i < esize && node_active(i))
                strcpy(gr2, "---");
            else
                *gr2 = 0;
        }
        if (*gr1 == 0 && *nname2 == 0 && *nname1 == 0 && *gr2 == 0)
            continue;

        fprintf(fp, "  %-7s %-23s %-23s %-8s\n", gr1, nname2, nname1, gr2);
    }

    // Print group names.
    {
        int i = 1;
        for ( ; i < psize; i++) {
            if (gd_groups[i].netname())
                break;
        }
        if (i < psize) {
            const char *types = "NTL";
            fprintf(fp, "\nGroup names obtained from labels (L), "
                "terminals (T), node name (N):\n\n");
            for ( ; i < psize; i++) {
                if (gd_groups[i].netname()) {
                    int n = gd_groups[i].node();
                    const char *nn = n >= 0 ? SCD()->nodeName(esdesc, n) : 0;

                    fprintf(fp, "  %-8d %c %-20s", i,
                        types[gd_groups[i].netname_origin()],
                        Tstring(gd_groups[i].netname()));
                    if (nn)
                        fprintf(fp, " N %s\n", nn);
                    else
                        fprintf(fp, "\n");
                }
            }
        }
    }

    // Show formal terminals, check for unplaced.
    if (esdesc->prpty(P_NODE)) {
        fprintf(fp, "\nFormal terminal group associations (F, location "
            "fixed by user placement):\n\n");
        for (int i = 0; i < esize; i++) {
            for (CDpin *p = pins_of_node(i); p; p = p->next()) {
                CDsterm *term = p->term();
                fprintf(fp, "  %-20s %c %4d", Tstring(term->name()),
                    term->is_fixed() ? 'F' : ' ', term->group());
                if (term->is_uninit()) {
                    // If this is simply an unconnected subcircuit terminal,
                    // it is not considered to be an error.

                    int st, dt;
                    tcount(conts_of_node(i), &st, &dt);
                    if (!dt && st <= 1)
                        fprintf(fp, " %s\n", "UNINITIALIZED (virtual)");
                    else {
                        fprintf(fp, " %s\n", "UNINITIALIZED");
                        lvs.bad_terms++;
                    }
                }
                else
                    fprintf(fp, "\n");
            }
        }
    }

    // Print permutation groups,
    {
        sSubcDesc *sd = EX()->findSubcircuit(gd_celldesc);
        if (sd && sd->num_groups() > 0) {
            sLstr lstr;
            lstr.add("\nPermutable contact groups detected:\n");
            int numg = sd->num_groups();
            for (int i = 0; i < numg; i++) {
                int tp = sd->group_type(i);
                lstr.add_c(' ');
                if (tp == PGnand)
                    lstr.add("NAND");
                else if (tp == PGnor)
                    lstr.add("NOR");
                else
                    lstr.add("TOPO");
                lstr.add_c('(');
                int gs = sd->group_size(i);
                int *a = sd->group_ary(i);
                for (int j = 0; j < gs; j++) {
                    lstr.add_c(' ');
                    lstr.add_i(a[j]);
                }
                lstr.add(" )\n");
            }
            fprintf(fp, "%s", lstr.string());
        }
    }

    // Print table of device associations.
    if (gd_devices) {
        fprintf(fp, "\nPhysical device associations:\n\n");
        for (sDevList *dl = gd_devices; dl; dl = dl->next()) {
            for (sDevPrefixList *p = dl->prefixes(); p; p = p->next()) {
                for (sDevInst *di = p->devs(); di; di = di->next()) {
                    fprintf(fp, "  %-8s%-6d:",  TstringNN(di->desc()->name()),
                        di->index());
                    if (di->dual()) {
                        char *instname = di->dual()->instance_name();
                        fprintf(fp, " %-8s%s\n",
                            Tstring(di->dual()->cdesc()->cellname()),
                            instname);
                        delete [] instname;
                    }
                    else
                        fprintf(fp, " <not associated>\n");

                    // print contacts
                    for (sDevContactInst *ci = di->contacts(); ci;
                            ci = ci->next()) {
                        int node = ci->group() >= 0 ?
                            gd_groups[ci->group()].node() : -1;
                        if (node >= 0) {
                            const char *nn = SCD()->nodeName(esdesc, node);
                            fprintf(fp, "    %-12s:   physical group %d, "
                                "electrical node (%d) %s\n",
                                TstringNN(ci->desc()->name()), ci->group(),
                                node, nn);
                        }
                        else
                            fprintf(fp, "    %-12s:   physical group %d, "
                                "electrical node <not found>\n",
                                TstringNN(ci->desc()->name()), ci->group());
                    }

                    // print parameters
                    sLstr lstr;
                    di->print_compare(&lstr, &lvs.val_cmp_errs,
                        &lvs.val_amb_cnt);
                    if (lstr.string())
                        fputs(lstr.string(), fp);
                }
            }
        }
    }

    // Print table of subckt associations.
    if (gd_subckts) {
        bool printed = false;
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            if (!printed) {
                fprintf(fp, "\nPhysical subcircuit associations:\n\n");
                printed = true;
            }
            for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                if (EX()->shouldFlatten(s->cdesc(), gd_celldesc))
                    continue;
                CDap ap(s->cdesc());
                if (ap.nx > 1 || ap.ny > 1) {
                    sprintf(tbuf, "[%d,%d]", s->ix(), s->iy());
                    char *iname = s->instance_name();
                    fprintf(fp, "  %-24s %-7s:",  iname, tbuf);
                    delete [] iname;
                }
                else {
                    char *iname = s->instance_name();
                    fprintf(fp, "  %-32s:", iname);
                    delete [] iname;
                }
                if (s->dual()) {
                    char *instname = s->dual()->instance_name();
                    fprintf(fp, " %-24s %s\n",
                        Tstring(s->dual()->cdesc()->cellname()), instname);
                    delete [] instname;
                }
                else
                    fprintf(fp, " <not associated>\n");

                // print contacts
                CDcbin cbin;
                CDcdb()->findSymbol(s->cdesc()->cellname(), &cbin);
                for (sSubcContactInst *c = s->contacts(); c; c = c->next())
                    print_subc_contact_lvs(fp, cbin, c);
                if (s->global_contacts())
                    fprintf(fp, "    --- global ---\n");
                for (sSubcContactInst *c = s->global_contacts(); c;
                        c = c->next())
                    print_subc_contact_lvs(fp, cbin, c);

            }
        }
    }

    // Unconnected physical instances.
    if (gd_celldesc->masters()) {
        fprintf(fp,
            "\nChecking for physical instances not connected to circuit.\n\n");
        CDm_gen mgen(gd_celldesc, GEN_MASTERS);
        int ndgt = CD()->numDigits();
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            bool found_m = false;
            for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
                if (mdesc->cellname() != sl->subs()->cdesc()->cellname())
                    continue;
                found_m = true;
                CDc_gen cgen(mdesc);
                for (CDc *cdesc = cgen.c_first(); cdesc;
                        cdesc = cgen.c_next()) {
                    bool found_c = false;
                    for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                        if (s->cdesc() != cdesc)
                            continue;
                        found_c = true;
                        break;
                    }
                    if (found_c)
                        continue;
                    if (EX()->shouldFlatten(cdesc, gd_celldesc))
                        continue;
                    fprintf(fp,
                        "  %-20s,  (%.*f,%.*f %.*f,%.*f)\n",
                        Tstring(mdesc->cellname()),
                        ndgt, MICRONS(cdesc->oBB().left),
                        ndgt, MICRONS(cdesc->oBB().bottom),
                        ndgt, MICRONS(cdesc->oBB().right),
                        ndgt, MICRONS(cdesc->oBB().top));
                    lvs.uc_cnt++;
                }
                break;
            }
            if (found_m)
                continue;

            CDs *sd = mdesc->celldesc();
            cGroupDesc *gd = sd->groups();
            if (!gd || gd->isempty())
                continue;
            if (gd->test_nets_only())
                continue;

            CDc_gen cgen(mdesc);
            for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
                if (EX()->shouldFlatten(cdesc, gd_celldesc))
                    continue;

                fprintf(fp,
                    "  %-20s,  (%.*f,%.*f %.*f,%.*f)\n",
                    Tstring(mdesc->cellname()),
                    ndgt, MICRONS(cdesc->oBB().left),
                    ndgt, MICRONS(cdesc->oBB().bottom),
                    ndgt, MICRONS(cdesc->oBB().right),
                    ndgt, MICRONS(cdesc->oBB().top));
                lvs.uc_cnt++;
            }
        }
        if (!lvs.uc_cnt)
            fprintf(fp, "  None.\n");
    }

    // Terminal references.
    fprintf(fp, "\nChecking per-group/node terminal references:\n\n");
    for (int i = 0; i < psize; i++)
        check_grp_node(i, lvs, fp);
    if (!lvs.bad_nets)
        fprintf(fp, "  No errors.\n");
    else
        fprintf(fp, "  %d of %d groups have terminal referencing errors.\n",
            lvs.bad_nets, psize);
    if (lvs.need_note1)
        fprintf(fp, "  [1] Not an error, subcircuit group is metal-only.\n");

    return (print_summary_lvs(fp, lvs));
}


// Add a message that will appear in the Summary lines of LVS output.
//
void
cGroupDesc::add_lvs_message(const char *fmt, ...)
{
    char buf[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 1024, fmt, args);
    va_end(args);

    stringlist *snew = new stringlist(lstring::copy(buf), 0);
    if (!gd_lvs_msgs)
        gd_lvs_msgs = snew;
    else {
        stringlist *sl = gd_lvs_msgs;
        while (sl->next)
            sl = sl->next;
        sl->next = snew;
    }
}


void
cGroupDesc::print_subc_contact_lvs(FILE *fp, const CDcbin &cbin,
    const sSubcContactInst *c)
{
    CDs *p_sd = cbin.phys();
    CDs *e_sd = cbin.elec();
    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    char tbuf[256];

    fprintf(fp, "    group %4d -> %-4d :  ", c->parent_group(),
        c->subc_group());

    int p_node = gd_groups[c->parent_group()].node();
    int s_node = -1;
    cGroupDesc *g = 0;
    if (p_sd) {
        g = p_sd->groups();
        if (g && !g->isempty())
            s_node = g->gd_groups[c->subc_group()].node();
        else
            g = 0;
    }
    const char *p_nn, *s_nn;
    if (p_node >= 0)
        p_nn = SCD()->nodeName(esdesc, p_node);
    else
        p_nn = "???";
    if (s_node >= 0 && e_sd)
        s_nn = SCD()->nodeName(e_sd, s_node);
    else
        s_nn = "???";

    if (p_node < 0) {
        if (gd_groups[c->parent_group()].global() &&
                gd_groups[c->parent_group()].netname())
            strcpy(tbuf, Tstring(gd_groups[c->parent_group()].netname()));
        else 
            strcpy(tbuf, "unconnected");
    }
    else
        sprintf(tbuf, "(%d) %s", p_node, p_nn);
    fprintf(fp, "%20s -> ", tbuf);

    if (s_node < 0) {
        if (g && g->gd_groups[c->subc_group()].global() &&
                g->gd_groups[c->subc_group()].netname())
            strcpy(tbuf,
                Tstring(g->gd_groups[c->subc_group()].netname()));
        else 
            strcpy(tbuf, "unconnected");
    }
    else
        sprintf(tbuf, "(%d) %s", s_node, s_nn);
    fprintf(fp, "%s\n", tbuf);
}


namespace {
    // List element for device instances.
    struct di_list
    {
        di_list(sDevInst *di, di_list *n)
            {
                dinst = di;
                next = n;
            }

        static void destroy(di_list *d)
            {
                while (d) {
                    di_list *dx = d;
                    d = d->next;
                    delete dx;
                }
            }

        sDevInst *dinst;
        di_list *next;
    };


    // Free a table of di_list structs.
    //
    void free_list_tab(SymTab *tab)
    {
        if (tab) {
            SymTabGen gen(tab, true);
            SymTabEnt *ent;
            while ((ent = gen.next()) != 0) {
                di_list::destroy((di_list*)ent->stData);
                delete ent;
            }
            delete tab;
        }
    }


    // Return a list of device instances that have a unresolved bulk
    // contact.
    //
    di_list *list_devs_defer_bulk(cGroupDesc *gd)
    {
        if (!gd)
            return (0);
        di_list *list = 0;
        for (sDevList *dv = gd->devices(); dv; dv = dv->next()) {
            for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                for (sDevInst *di = p->devs(); di; di = di->next()) {
                    bool fdef = false;
                    for (sDevContactInst *ci = di->contacts(); ci;
                            ci = ci->next()) {
                        if (ci->desc()->level() == BC_defer) {
                            fdef = true;
                            if (ci->group() < 0)
                                list = new di_list(di, list);
                            break;
                        }
                    }
                    if (!fdef)
                       break;
                }
            }
        }
        return (list);
    }
}


// Go through the entire hierarchy and look for unresolved BC_defer
// bulk contacts.  See if these can be resolved, and if not add an
// entry to the returned list.  This is called in LVS.
//
sBcErr *
cGroupDesc::check_stamping()
{
    SymTab *done_tab = new SymTab(false, false);
    list_callers(0, done_tab);

    SymTab *list_tab = 0;
    SymTabGen gen(done_tab, true);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        CDs *sd = (CDs*)ent->stTag;
        di_list *dil = list_devs_defer_bulk(sd->groups());
        if (dil) {
            if (!list_tab)
                list_tab = new SymTab(false, false);
            list_tab->add((uintptr_t)sd, dil, false);
        }
        delete ent;
    }
    delete done_tab;
    if (list_tab) {
        cTfmStack stk;
        sBcErr *e = check_stamping_rc(this, list_tab, stk);
        free_list_tab(list_tab);
        return (e);
    }
    return (0);
}


sBcErr *
cGroupDesc::check_stamping_rc(cGroupDesc *topdesc, SymTab *list_tab,
    cTfmStack &stk)
{
    sBcErr *er0 = 0, *erend = 0;
    for (sSubcList *sc = gd_subckts; sc; sc = sc->next()) {
        for (sSubcInst *su = sc->subs(); su; su = su->next()) {
            CDs *msd = su->cdesc()->masterCell();
            if (!msd)
                continue;
            cGroupDesc *gd = msd->groups();
            if (!gd)
                continue;

            stk.TPush();
            stk.TApplyTransform(su->cdesc());
            CDap ap(su->cdesc());
            stk.TTransMult(su->ix()*ap.dx, su->iy()*ap.dy);
            stk.TPremultiply();
            sBcErr *e = gd->check_stamping_rc(topdesc, list_tab, stk);
            stk.TPop();
            if (e) {
                if (!er0)
                    er0 = erend = e;
                else {
                    while (erend->next())
                        erend = erend->next();
                    erend->set_next(e);
                }
            }
        }
    }
    di_list *dil = (di_list*)SymTab::get(list_tab, (uintptr_t)gd_celldesc);
    if (dil == (di_list*)ST_NIL)
        return (0);
    for ( ; dil; dil = dil->next) {
        sDevInst *di = dil->dinst;

        BBox bBB(*di->bBB());
        stk.TBB(&bBB, 0);
        sDevContactInst *ci = di->contacts();
        while (ci && ci->desc()->level() != BC_defer)
            ci = ci->next();
        if (!ci)
            continue;

        XIrt xrt;
        if (topdesc->has_bulk_contact(di->desc(), &bBB, ci->desc(), &xrt))
            continue;

        // badone!
        sBcErr *e = new sBcErr(gd_celldesc, di->desc(), &bBB, 0);
        if (!er0)
            er0 = erend = e;
        else {
            while (erend->next())
                erend = erend->next();
            erend->set_next(e);
        }
    }
    return (er0);
}


// Check the terminal references.
//
void
cGroupDesc::check_grp_node(int grp, sLVSstat &lvs, FILE *fp)
{
#define GROUP_ASSOC_ERROR   0x1
#define NEED_NOTE_1         0x2

    sGroup *g = group_for(grp);
    if (!g || g->node() <= 0 || !g->has_terms())
        return;

    const char *hdr = "  Checking group %d (node %d):\n";
    bool hdr_printed = false;
    int retval = 0;
    int tcnt = 0;
    for (CDcont *t = conts_of_node(g->node()); t; t = t->next())
        tcnt++;
    CDcterm **terms = new CDcterm*[tcnt];
    tcnt = 0;
    for (CDcont *t = conts_of_node(g->node()); t; t = t->next())
        terms[tcnt++] = t->term();

    for (sDevContactList *dc = g->device_contacts(); dc; dc = dc->next()) {
        sDevInst *di = dc->contact()->dev();
        bool found = false;
        for (int i = 0; i < tcnt; i++) {
            if (!terms[i] || !terms[i]->instance())
                continue;
            if (di->dual() && di->dual()->cdesc() != terms[i]->instance())
                continue;
            if (di->desc()->name() != terms[i]->instance()->cellname())
                continue;
            if (di->desc()->is_permute(dc->contact()->desc()->name())) {
                if (di->desc()->is_permute(terms[i]->master_name())) {
                    found = true;
                    terms[i] = 0;
                    break;
                }
            }
            else if (terms[i]->master_name() ==
                    dc->contact()->desc()->name()) {
                found = true;
                terms[i] = 0;
                break;
            }
        }
        if (!found) {
            if (!hdr_printed) {
                hdr_printed = true;
                fprintf(fp, hdr, grp, g->node());
            }
            fprintf(fp,
            "    Physical device contact %s %d %s not connected to node.\n",
                TstringNN(di->desc()->name()), di->index(),
                TstringNN(dc->contact()->desc()->name()));
            retval |= GROUP_ASSOC_ERROR;
        }
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
            if (!terms[i] || !terms[i]->instance())
                continue;
            if (subc->dual() && subc->dual()->cdesc() != terms[i]->instance())
                continue;
            if (sd->cellname() != terms[i]->instance()->cellname())
                continue;
            int subg = terms[i]->master_group();
            if (subg < 0) {
                // terminal not placed, must be a non-connected net
                if (ci->subc_group() >= 0 &&
                        ci->subc_group() < gd->gd_asize) {
                    sGroup *gp = gd->group_for(ci->subc_group());
                    if (!gp->subc_contacts() && !gp->device_contacts()) {
                        // consistent
                        found = true;
                        terms[i] = 0;
                        break;
                    }
                    if (gp->global() && gp->netname()) {
                        // global net, not an error
                        found = true;
                        terms[i] = 0;
                        break;
                    }
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

        if (!found) {
            if (!hdr_printed) {
                hdr_printed = true;
                fprintf(fp, hdr, grp, g->node());
            }

            // We have found a group without a corresponding node.  If
            // the corresponding subckt group is metal-only, then this
            // will not be an error.  The metal in the subckt is
            // facilitating contacts at a higher level only, and
            // really doesn't belong in the cell.

            bool noterr = ci->is_wire_only();
            if (noterr)
                retval |= NEED_NOTE_1;
            else
                retval |= GROUP_ASSOC_ERROR;
            char *iname = ci->subc()->instance_name();
            fprintf(fp,
                "    Physical contact to group %d in %s not "
                "connected to node%s\n", ci->subc_group(), iname,
                noterr ? " (OK [1])." : ".");
            delete [] iname;
        }
    }

    // now account for formal terminal connections
    int pcnt = 0;
    for (CDpin *p = pins_of_node(g->node()); p; p = p->next())
        pcnt++;
    CDsterm **pins = new CDsterm*[pcnt];
    pcnt = 0;
    for (CDpin *p = pins_of_node(g->node()); p; p = p->next())
        pins[pcnt++] = p->term();

    for (CDpin *p = g->termlist(); p; p = p->next()) {
        bool found = false;
        for (int i = 0; i < pcnt; i++) {
            if (pins[i] == p->term()) {
                found = true;
                pins[i] = 0;
                break;
            }
        }
        if (!found) {
            if (!hdr_printed) {
                hdr_printed = true;
                fprintf(fp, hdr, grp, g->node());
            }
            fprintf(fp,
            "    Physical formal terminal %s not connected to node.\n",
                Tstring(p->term()->name()));
            retval |= GROUP_ASSOC_ERROR;
        }
    }
    delete [] pins;

    // residual electrical terminals not referenced in group
    for (int i = 0; i < tcnt; i++) {
        if (!terms[i])
            continue;

        CDc *cd = terms[i]->instance();
        if (cd) {

            // If this corresponds to a bulk device terminal which was
            // not resolved, it is not an error here.  The unresolved
            // BC_defer terms are reported in the global stamping
            // report at the top level.
            //
            CDp_cnode *pn = terms[i]->node_prpty();
            if (pn) {
                sDevInst *di = find_dual_dev(cd, terms[i]->inst_index());
                if (di) {
                    CDnetName name = 0;
                    CDs *sd = cd->masterCell();
                    if (sd) {
                        CDp_snode *ps = (CDp_snode*)sd->prpty(P_NODE);
                        for ( ; ps; ps = ps->next()) {
                            if (ps->index() == pn->index()) {
                                name = ps->get_term_name();
                                break;
                            }
                        }
                    }
                    if (name) {
                        sDevContactInst *ci = di->contacts();
                        for ( ; ci; ci = ci->next()) {
                            if (name == ci->desc()->name()) {
                                if (!ci->cont_ok() &&
                                        ci->desc()->level() != BC_immed) {
                                    if (ci->desc()->level() == BC_defer)
                                        lvs.poss_bad_conts++;
                                    terms[i] = 0;
                                }
                                break;
                            }
                        }
                    }
                    if (!terms[i])
                        continue;
                }
            }
        }

        if (!hdr_printed) {
            hdr_printed = true;
            fprintf(fp, hdr, grp, g->node());
        }
        fprintf(fp,
        "    Electrical terminal %s not connected to group.\n",
            Tstring(terms[i]->name()));
        retval |= GROUP_ASSOC_ERROR;
    }

    delete [] terms;

    if (retval & GROUP_ASSOC_ERROR)
        lvs.bad_nets++;
    if (retval & NEED_NOTE_1)
        lvs.need_note1 = true;
}


LVSresult
cGroupDesc::print_summary_lvs(FILE *fp, sLVSstat &lvs)
{
    fprintf(fp, "\nSummary:\n\n");

    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    int psize = nextindex();
    int esize = gd_etlist ? gd_etlist->size() : 0;
    bool something_printed = false;

    if (lvs.bad_terms) {
        fprintf(fp,
            "  Found %d unplaced formal %s.\n", lvs.bad_terms,
            lvs.bad_terms > 1 ? "terminals" : "terminal");
        something_printed = true;
    }
    if (lvs.bad_nets) {
        fprintf(fp,
            "  Found %d %s with terminal referencing errors.\n",
            lvs.bad_nets, lvs.bad_nets > 1 ? "groups" : "group");
        something_printed = true;
    }
    if (lvs.bad_conts) {
        fprintf(fp,
            "  Found %d globally unresolved bulk %s.\n",
            lvs.bad_conts, lvs.bad_conts > 1 ? "contacts" : "contact");
        something_printed = true;
    }
    if (lvs.poss_bad_conts) {
        fprintf(fp,
            "  Found %d locally unresolved bulk %s (not an error here).\n",
            lvs.poss_bad_conts,
            lvs.poss_bad_conts > 1 ? "contacts" : "contact");
        something_printed = true;
    }
    for (int i = 1; i < psize; i++) {
        sGroup &g = gd_groups[i];
        if (g.has_net_or_terms() && g.node() < 0) {
            CDnetName nn = g.netname();
            if (g.global() && nn) {
                fprintf(fp, "  Physical group %d is global net %s.\n", i,
                    Tstring(nn));
                something_printed = true;
            }
            else if (g.has_terms() && !g.unas_wire_only()) {
                fprintf(fp, "  Physical group %d is not associated.\n", i);
                something_printed = true;
                lvs.gp_unassoc++;
            }
            else {
                fprintf(fp,
                    "  Physical group %d is wire only (not an error).\n", i);
                something_printed = true;
            }
        }
    }
    for (int i = 1; i < esize; i++) {
        if (node_active(i) && group_of_node(i) < 0) {
            const char *nn = SCD()->nodeName(esdesc, i);
            fprintf(fp, "  Electrical net %s is not associated", nn);
            something_printed = true;
            lvs.nt_unassoc++;

            // If this is simply an unconnected subcircuit terminal,
            // it is not considered to be an error.
            int st, dt;
            tcount(conts_of_node(i), &st, &dt);
            if (!dt && st <= 1) {
                fprintf(fp, "  ( virtual net - not an error ).\n");
                lvs.nt_unassoc--;
            }
            else
                fprintf(fp, ".\n");
        }
    }
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *s = p->devs(); s; s = s->next()) {
                if (!s->dual()) {
                    fprintf(fp, "  Physical device %s %d is not associated.\n",
                        TstringNN(s->desc()->name()), s->index());
                    something_printed = true;
                    lvs.pd_unassoc++;
                }
            }
        }
        for (sEinstList *c = dv->edevs(); c; c = c->next()) {
            if (!c->dual_dev()) {
                CDc *cd = c->cdesc();
                char *instname = cd->getElecInstName(c->cdesc_index());
                if (lstring::prefix(WIRECAP_PREFIX, instname)) {
                    // This is a wire capacitor, which has no physical dual
                    // Shouldn't get here, wire caps should be in extras
                    if (check_wire_cap(cd, instname, fp))
                        lvs.ed_unassoc++;
                }
                else {
                    fprintf(fp,
                        "  Electrical device %s (%s) is not associated.\n",
                        instname, Tstring(cd->cellname()));
                    something_printed = true;
                    lvs.ed_unassoc++;
                }
                delete [] instname;
            }
        }
    }
    if (gd_extra_devs) {
        for (sEinstList *c = gd_extra_devs; c; c = c->next()) {
            CDc *cd = c->cdesc();
            char *instname = cd->getElecInstName(c->cdesc_index());
            if (lstring::prefix(WIRECAP_PREFIX, instname)) {
                // This is a wire capacitor, which has no physical dual
                if (check_wire_cap(cd, instname, fp))
                    lvs.ed_unassoc++;
            }
            else {
                fprintf(fp,
                    "  Electrical device %s (%s) is not associated.\n",
                    instname, Tstring(cd->cellname()));
                something_printed = true;
                lvs.ed_unassoc++;
            }
            delete [] instname;
        }
    }
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (EX()->shouldFlatten(s->cdesc(), gd_celldesc))
                continue;
            if (!s->dual()) {
                CDap ap(s->cdesc());
                char *iname = s->instance_name();
                if (ap.nx > 1 || ap.ny > 1) {
                    fprintf(fp,
                        "  Physical subcircuit %s [%d,%d] is not associated.\n",
                        iname, s->ix(), s->iy());
                }
                else {
                    fprintf(fp, "  Physical subcircuit %s is not associated.\n",
                        iname);
                }
                delete [] iname;
                something_printed = true;
                lvs.ps_unassoc++;
            }
        }
        for (sEinstList *s = sl->esubs(); s; s = s->next()) {
            if (!s->dual_subc()) {
                char *instname = s->cdesc()->getElecInstName(s->cdesc_index());
                fprintf(fp,
                    "  Electrical subcircuit %s (%s) is not associated.\n",
                    instname, Tstring(s->cdesc()->cellname()));
                delete [] instname;
                something_printed = true;
                lvs.es_unassoc++;
            }
        }
    }
    if (gd_extra_subs) {
        for (sEinstList *c = gd_extra_subs; c; c = c->next()) {
            char *instname = c->cdesc()->getElecInstName(c->cdesc_index());
            fprintf(fp,
                "  Electrical subcircuit %s (%s) is not associated.\n",
                instname, Tstring(c->cdesc()->cellname()));
            delete [] instname;
            something_printed = true;
            lvs.es_unassoc++;
        }
    }
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *s = p->devs(); s; s = s->next()) {
                if (!s->dual()) {
                    for (sDevContactInst *c = s->contacts(); c;
                            c = c->next()) {
                        if (gd_groups[c->group()].node() > 0)
                            lvs.dc_assoc++;
                    }
                }
                else {
                    for (sDevContactInst *c = s->contacts(); c;
                            c = c->next()) {
                        sGroup &g = gd_groups[c->group()];
                        if (g.node() >= 0)
                            continue;
                        if (g.global() && g.netname())
                            continue;
                        lvs.dc_unassoc++;
                    }
                }
            }
        }
    }
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (EX()->shouldFlatten(s->cdesc(), gd_celldesc))
                continue;
            if (!s->dual()) {
                for (sSubcContactInst *c = s->contacts(); c; c = c->next()) {
                    if (gd_groups[c->parent_group()].node() >= 0)
                        lvs.sc_assoc++;
                }
            }
            else {
                for (sSubcContactInst *c = s->contacts(); c; c = c->next()) {
                    sGroup &g = gd_groups[c->parent_group()];
                    if (g.node() >= 0)
                        continue;
                    if (g.global() && g.netname())
                        continue;
                    if (g.unas_wire_only())
                        continue;
                    lvs.sc_unassoc++;
                }
            }
        }
    }
    if (something_printed)
        fprintf(fp, "\n");

    const char *msg = "  %d %s not associated.\n";

    if (lvs.uc_cnt) {
        if (CDvdb()->getVariable(VA_LvsFailNoConnect)) {
            fprintf(fp,
                "  %d unconnected physical %s (enforced LVS failure).\n",
                lvs.uc_cnt, lvs.uc_cnt == 1 ? "instance" : "instances");
        }
        else {
            fprintf(fp, "  %d unconnected physical %s.\n", lvs.uc_cnt,
                lvs.uc_cnt == 1 ? "instance" : "instances");
            lvs.uc_cnt = 0;
        }
    }
    if (lvs.gp_unassoc)
        fprintf(fp, msg, lvs.gp_unassoc,
            lvs.gp_unassoc == 1 ? "group" : "groups");
    if (lvs.dc_unassoc)
        fprintf(fp, "  %d associated device %s to an unassociated group.\n",
            lvs.dc_unassoc, lvs.dc_unassoc == 1 ? "contact" : "contacts");
    if (lvs.sc_unassoc)
        fprintf(fp,
            "  %d associated subcircuit %s to an unassociated group.\n",
            lvs.sc_unassoc, lvs.sc_unassoc == 1 ? "contact" : "contacts");
    if (lvs.nt_unassoc)
        fprintf(fp, msg, lvs.nt_unassoc, lvs.nt_unassoc == 1 ? "net" : "nets");
    if (lvs.pd_unassoc)
        fprintf(fp, msg, lvs.pd_unassoc,
            lvs.pd_unassoc == 1 ? "physical device" : "physical devices");
    if (lvs.dc_assoc)
        fprintf(fp, "  %d unassociated device %s to an associated group.\n",
            lvs.dc_assoc, lvs.dc_assoc == 1 ? "contact" : "contacts");
    if (lvs.ed_unassoc)
        fprintf(fp, msg, lvs.ed_unassoc,
            lvs.ed_unassoc == 1 ? "electrical device" : "electrical devices");
    if (lvs.ps_unassoc)
        fprintf(fp, msg, lvs.ps_unassoc, lvs.ps_unassoc == 1 ?
            "physical subcircuit" : "physical subcircuits");
    if (lvs.sc_assoc)
        fprintf(fp,
            "  %d unassociated subcircuit %s to an associated group.\n",
            lvs.sc_assoc, lvs.sc_assoc == 1 ? "contact" : "contacts");
    if (lvs.es_unassoc)
        fprintf(fp, msg, lvs.es_unassoc, lvs.es_unassoc == 1 ?
            "electrical subcircuit" : "electrical subcircuits");
    if (lvs.val_cmp_errs)
        fprintf(fp, "  %d value comparison %s.\n", lvs.val_cmp_errs,
            lvs.val_cmp_errs == 1 ? "difference" : "differences");
    if (lvs.val_amb_cnt)
        fprintf(fp, "  %d unavailable value comparisons.\n", lvs.val_amb_cnt);

    for (int i = 1; i < psize; i++) {
        sGroup &g = gd_groups[i];
        if (g.node() < 0 && g.split_group() >= 0) {
            int node = gd_groups[g.split_group()].node();
            fprintf(fp, 
                "  Unassociated group %d looks like split of group "
                "%d (node %d).\n",
                i, g.split_group(), node);
        }
    }
    for (stringlist *sl = gd_lvs_msgs; sl; sl = sl->next)
        fprintf(fp, "%s\n", sl->string);

    if (!lvs.gp_unassoc && !lvs.pd_unassoc && !lvs.ps_unassoc)
        fprintf(fp,
    "  All physical nets, devices, and subcircuits have been associated.\n");
    if (!lvs.nt_unassoc && !lvs.ed_unassoc && !lvs.es_unassoc)
        fprintf(fp,
    "  All electrical nets, devices, and subcircuits have been associated.\n");

    const char *fmt = "\nLVS of cell %s %s.\n\n";
    if (lvs.bad_terms + lvs.bad_nets + lvs.bad_conts + lvs.uc_cnt +
            lvs.nt_unassoc + lvs.ed_unassoc + lvs.es_unassoc +
            lvs.dc_unassoc + lvs.ps_unassoc + lvs.pd_unassoc +
            lvs.sc_unassoc + lvs.dc_assoc + lvs.sc_assoc) {
        fprintf(fp, fmt, Tstring(gd_celldesc->cellname()), "FAILED");
        return (LVSfail);
    }
    if (lvs.val_cmp_errs) {
        fprintf(fp, fmt, Tstring(gd_celldesc->cellname()),
            "PASSED, but with PARAMETER DIFFERENCES");
        return (LVStopok);
    }
    if (lvs.val_amb_cnt) {
        fprintf(fp, fmt, Tstring(gd_celldesc->cellname()),
            "PASSED, but with UNAVAILABLE PARAMETERS");
        return (LVSap);
    }
    fprintf(fp, fmt, Tstring(gd_celldesc->cellname()), "CLEAN");
    return (LVSclean);
}


// The cd argument is an electrical wire cap instance with name in cname.
// Check this against the physical capacitance and print the result if
// no match or error.  Return true if no match or error.
//
bool
cGroupDesc::check_wire_cap(CDc *cd, const char *cname, FILE *fp)
{
    CDp_node *pn = (CDp_node*)cd->prpty(P_NODE);
    if (pn && pn->enode() == 0)
        pn = pn->next();
    if (!pn) {
        fprintf(fp, "  wire cap %s: (internal) missing node property!\n",
            cname);
        return (true);
    }
    if (pn->enode() < 1) {
        fprintf(fp, "  wire cap %s: no connection!\n", cname);
        return (true);
    }
    int g = group_of_node(pn->enode());
    if (g < 1) {
        fprintf(fp, "  wire cap %s: no associated group (node=%d)\n", cname,
            pn->enode());
        return (true);
    }
    double pcap = gd_groups[g].capac();
    CDp_user *pv = (CDp_user*)cd->prpty(P_VALUE);
    if (!pv) {
        fprintf(fp, "  wire cap %s: missing VALUE property\n", cname);
        return (true);
    }
    char *string = hyList::string(pv->data(), HYcvPlain, true);
    double ecap;
    if (!string || sscanf(string, "%lf", &ecap) != 1) {
        fprintf(fp, "  wire cap %s: bad value %s\n", cname,
            string ? string : "(null)");
        return (true);
    }
    delete [] string;
    bool ok = false;
    double u = fabs(pcap - ecap);
    if (u == 0.0)
        ok = true;
    else {
        double v = 0.5*(fabs(pcap) + fabs(ecap));
#define WIRECAP_PREC 100.0
        v /= WIRECAP_PREC;
        if (u <= v)
            ok = true;
    }
    if (!ok) {
        fprintf(fp, "  wire cap %s mismatch, physical=%.3e electrical=%.3e\n",
            cname, pcap, ecap);
        return (true);
    }
    return (false);
}

