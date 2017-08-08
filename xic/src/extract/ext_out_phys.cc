
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
#include "sced.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "fio_gencif.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "tech.h"
#include "errorlog.h"
#include "spnumber/spnumber.h"

/*========================================================================*
 *
 *  Functions for dumping extracted physical netlist output.
 *
 *========================================================================*/


// Main function to dump netlist information extracted from the
// layout.
//
bool
cExt::dumpPhysNetlist(FILE *fp, CDs *sdesc, sDumpOpts *opts)
{
    if (!sdesc)
        return (true);
    if (sdesc->isElectrical()) {
        sdesc = CDcdb()->findCell(sdesc->cellname(), Physical);
        if (!sdesc)
            return (true);
    }

    dspPkgIf()->SetWorking(true);
    XM()->OpenFormatLib(EX_PNET_FORMAT);
    SymTab *tab = new SymTab(false, false);
    bool ret = false;
    if (!skipExtract(sdesc) && extract(sdesc)) {
        if (opts->spice_print_mode() == PSPM_physical ||
                (opts->spice_print_mode() != PSPM_physical &&
                associate(sdesc))) {
            bool format_given = opts->isset(opt_atom_net) ||
                opts->isset(opt_atom_devs) || opts->isset(opt_atom_spice);
            if (!format_given) {
                for (int i = opts->user_button(); i < opts->num_buttons();
                        i++) {
                    if (opts->button(i)->is_set()) {
                        format_given = true;
                        break;
                    }
                }
            }
            if (!format_given)
                Log()->ErrorLog(mh::NetlistCreation,
                    "No output format was selected.");
            else
                ret = dump_phys_recurse(fp, sdesc, sdesc, opts->depth(), opts,
                    tab);
        }
    }
    delete tab;
    dspPkgIf()->SetWorking(false);
    return (ret);
}


// Recursive private core of DumpPhysNetlist.
//
bool
cExt::dump_phys_recurse(FILE *fp, CDs *topsdesc, CDs *sdesc, int depth,
    sDumpOpts *opts, SymTab *tab)
{
    cGroupDesc *gd = sdesc->groups();
    if (!opts->isset(opt_atom_btmup)) {
        if (sdesc == topsdesc) {
            tab->add((unsigned long)sdesc, 0, false);
            if (!gd || (gd->test_nets_only() && !opts->isset(opt_atom_all))) {
                if (opts->isset(opt_atom_net)) {
                    fprintf(fp, "## %s (empty cell)\n",
                        Tstring(sdesc->cellname()));
                }
                return (true);
            }
        }
        if (!gd->print_phys(fp, topsdesc, depth, opts))
            return (false);
    }
    if (depth > 0) {
        CDm_gen mgen(sdesc, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            if (!mdesc->hasInstances())
                continue;
            CDs *msdesc = mdesc->celldesc();
            if (!msdesc)
                continue;
            if (SymTab::get(tab, (unsigned long)msdesc) != ST_NIL)
                continue;
            if (!opts->isset(opt_atom_all)) {
                cGroupDesc *tgd = msdesc->groups();
                if (tgd && tgd->test_nets_only()) {
                    tab->add((unsigned long)msdesc, 0, false);
                    continue;
                }

                // This is a little subtle.  We need to check all of
                // the instances of this master, if we ever find one
                // that is not flattened recurse into it and add it to
                // the table.
                //
                bool needit = false;
                CDc_gen cgen(mdesc);
                for (CDc *cd = cgen.c_first(); cd; cd = cgen.c_next()) {
                    if (!gd->in_flatten_list(cd)) {
                        needit = true;
                        break;
                    }
                }
                if (!needit)
                    continue;
            }
            tab->add((unsigned long)msdesc, 0, false);
            if (!dump_phys_recurse(fp, topsdesc, msdesc, depth-1, opts,  tab))
                return (false);
        }
    }
    if (opts->isset(opt_atom_btmup)) {
        if (sdesc == topsdesc) {
            tab->add((unsigned long)sdesc, 0, false);
            if (!gd || (gd->test_nets_only() && !opts->isset(opt_atom_all))) {
                if (opts->isset(opt_atom_net)) {
                    fprintf(fp, "## %s (empty cell)\n",
                        Tstring(sdesc->cellname()));
                }
                return (true);
            }
        }
        if (!gd->print_phys(fp, topsdesc, depth, opts))
            return (false);
    }
    return (true);
}
// End of cExt functions.


// Print physical netlist information.
//
bool
cGroupDesc::print_phys(FILE *fp, CDs *topsdesc, int, sDumpOpts *opts)
{
    if (opts->isset(opt_atom_net))
        print_groups(fp, opts);
    if (isempty())
        return (true);
    bool do_spice = true;
    if (opts->isset(opt_atom_devs)) {
        fprintf(fp, "Device and subcircuit instances in physical cell %s:\n",
            Tstring(gd_celldesc->cellname()));

        int cnt = print_devs_subs(fp, opts);
        if (!cnt) {
            fprintf(fp, "none found\n");
            if (!EX()->skipExtract(gd_celldesc))
                // print a dummy .subckt line for opaque cells
                do_spice = false;
        }
        fprintf(fp, "\n");
    }
    if (opts->isset(opt_atom_spice) && do_spice) {
        // Dump a SPICE deck created from physical layout
        if (!print_spice(fp, topsdesc, opts))
            return (false);
    }
    for (int i = opts->user_button(); i < opts->num_buttons(); i++) {
        if (opts->button(i)->is_set())
            print_formatted(fp, opts->button(i)->name(), opts);
    }
    return (true);
}


// Print the nets.
//
void
cGroupDesc::print_groups(FILE *fp, sDumpOpts *opts)
{
    fprintf(fp, "############  %s\n", XM()->IdString());
    fprintf(fp, "Physical netlist of cell %s\n",
        Tstring(gd_celldesc->cellname()));
    if (isempty()) {
        fprintf(fp, "No physical nets found.\n");
        return;
    }

    int i0 = 0;
    if (!has_net_or_terms(0))
        i0 = 1;
    int i = nextindex();
    if (i0 == i) {
        fprintf(fp, "No physical nets found.\n");
        return;
    }
    if (i - i0 == 1)
        fprintf(fp, "Using group %d\n", i0);
    else
        fprintf(fp, "Using groups %d - %d\n", i0, i-1);

    for (i = 0; i < gd_asize; i++) {
        sGroup &g = gd_groups[i];

        if (g.termlist() || g.device_contacts() || g.subc_contacts()) {
            if (!g.net() && i > 0) {
                if (g.netname())
                    fprintf(fp, "Group %d (%s, virtual):\n", i,
                        Tstring(g.netname()));
                else
                    fprintf(fp, "Group %d (virtual):\n", i);
            }
            else {
                if (g.netname())
                    fprintf(fp, "Group %d (%s):\n", i, Tstring(g.netname()));
                else
                    fprintf(fp, "Group %d:\n", i);
            }
        }

        char buf[256];
        if (g.capac() != 0.0) {
            sprintf(buf, "Capacitance %1.3e", g.capac());
            Gen.Comment(fp, buf);
        }
        if (g.net()) {
            int numl = CDldb()->layersUsed(Physical);
            for (int ix = 1; ix < numl; ix++) {
                CDl *ld = CDldb()->layer(ix, Physical);
                if (!ld->isConductor())
                    continue;
                bool lyr_p = false;
                Zlist *z0 = 0;
                for (CDol *ol = g.net()->objlist(); ol; ol = ol->next) {
                    if (ol->odesc->ldesc() != ld)
                        continue;
                    if (!lyr_p) {
                        lyr_p = true;
                        if (opts->isset(opt_atom_geom))
                            Gen.Layer(fp, ld->name());
                    }
                    Zlist *z = 0;
                    switch (ol->odesc->type()) {
                    case CDBOX:
                        if (opts->isset(opt_atom_geom))
                            Gen.Box(fp, ol->odesc->oBB());
                        z = ol->odesc->toZlist();
                        break;
                    case CDPOLYGON:
                        if (opts->isset(opt_atom_geom))
                            Gen.Polygon(fp, OPOLY(ol->odesc)->points(),
                                OPOLY(ol->odesc)->numpts());
                        z = ol->odesc->toZlist();
                        break;
                    case CDWIRE:
                        if (opts->isset(opt_atom_geom))
                            Gen.Wire(fp, OWIRE(ol->odesc)->wire_width(),
                                OWIRE(ol->odesc)->wire_style(),
                                OWIRE(ol->odesc)->points(),
                                OWIRE(ol->odesc)->numpts());
                        z = ol->odesc->toZlist();
                        break;
                    }
                    if (z) {
                        Zlist *ze = z;
                        while (ze->next)
                            ze = ze->next;
                        ze->next = z0;
                        z0 = z;
                    }
                }
                if (z0) {
                    PolyList *p0 = Zlist::to_poly_list(z0);
                    double area = 0.0, perim = 0.0;
                    for (PolyList *p = p0; p; p = p->next) {
                        area += p->po.area();
                        perim += MICRONS(p->po.perim());
                    }
                    sprintf(buf, "%s: area=%1.3e perim=%1.3e", ld->name(),
                        area, perim);
                    Gen.Comment(fp, buf);
                    PolyList::destroy(p0);
                }
            }
        }

        for (CDpin *p = g.termlist(); p; p = p->next()) {
            if (!p->term() || p->term()->instance())
                continue;
            if (p->term()->name())
                fprintf(fp, "  Cell terminal %s\n",
                    Tstring(p->term()->name()));
            else
                fprintf(fp, "  Cell terminal <unnamed>\n");
        }
        for (sDevContactList *c = g.device_contacts(); c; c = c->next()) {
            fprintf(fp, "  %s %d: %s\n",
                TstringNN(c->contact()->dev()->desc()->name()),
                c->contact()->dev()->index(),
                TstringNN(c->contact()->desc()->name()));
        }
        for (sSubcContactList *s = g.subc_contacts(); s; s = s->next()) {
            // ignore contacts to ignored or flattened subcells
            CDc *cdesc = s->contact()->subc()->cdesc();
            if ((!opts || !opts->isset(opt_atom_all)) &&
                    EX()->shouldFlatten(cdesc, gd_celldesc))
                continue;

            CDp *pd = cdesc->prpty(XICP_INST);
            char *iname = s->contact()->subc()->instance_name();
            if (pd && pd->string() && *pd->string())
                fprintf(fp, "  %s (%s): ", iname, pd->string());
            else
                fprintf(fp, "  %s: ", iname);
            delete [] iname;

            CDs *msdesc = cdesc->masterCell(true);
            bool didit = false;
            int grp = s->contact()->subc_group();
            if (msdesc) {
                cGroupDesc *gd = msdesc->groups();
                if (gd) {
                    bool has_term, has_conflict;
                    const char *nm = gd->group_name(grp, &has_term,
                        &has_conflict);
                    if (nm) {
                        if (has_conflict) {
                            fprintf(fp, "%d (net %s term %s CONFLICT)\n", grp,
                                nm, Tstring(gd->group_for(grp)->netname()));
                        }
                        else if (has_term)
                            fprintf(fp, "%d (term %s)\n", grp, nm);
                        else
                            fprintf(fp, "%d (net %s)\n", grp, nm);
                        didit = true;
                    }
                }
            }
            if (!didit)
                fprintf(fp, "%d\n", grp);
        }
    }
    fprintf(fp, "\n");
}


// Print a list of devices and subcircuits, return a count of the
// devices plus subcircuits found.
//
int
cGroupDesc::print_devs_subs(FILE *fp, sDumpOpts *opts)
{
    sort_devs(false);
    sort_subs(false);

    int cnt = 0;

    // dump the devices
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                di->print(fp, opts->isset(opt_atom_verbose));
                cnt++;
            }
        }
    }

    // dump the subcircuit calls
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (!s->contacts())
                continue;
            CDp *pd = s->cdesc()->prpty(XICP_INST);
            if (pd && pd->string() && *pd->string())
                fprintf(fp, "%s %s\n", Tstring(s->cdesc()->cellname()),
                    pd->string());
            else {
                char *iname = s->instance_name();
                fprintf(fp, "%s\n", iname);
                delete [] iname;
            }
            cnt++;
        }
    }
    return (cnt);
}


// Print a SPICE listing.
//
bool
cGroupDesc::print_spice(FILE *fp, CDs *topsdesc, sDumpOpts *opts)
{
    {
        cGroupDesc *gdt = this;
        if (!gdt)
            return (true);
    }
    fprintf(fp, "* SPICE listing of physical cell %s\n",
        Tstring(gd_celldesc->cellname()));

    // Control the global state for how to print group names.
    struct ign_ctrl {
        ign_ctrl(bool state)
            {
                ign_bak = EX()->isIgnoreGroupNames();
                EX()->setIgnoreGroupNames(state);
            }

        ~ign_ctrl() { EX()->setIgnoreGroupNames(ign_bak); }

    private:
        bool ign_bak;
    } _ign(opts->isset(opt_atom_labels));

    sLstr lstr;
    char buf[256];
    bool printsc = false;
    if (topsdesc == gd_celldesc) {
        sSubcDesc *scd = EX()->findSubcircuit(gd_celldesc);
        if (scd) {
            fprintf(fp, ".subckt %s",
                Tstring(gd_celldesc->cellname()));
            for (unsigned int i = 0; i < scd->num_contacts(); i++) {
                int grp = scd->contact(i);
                const char *grpname = group_name(grp, &lstr);
                fprintf(fp, " %s", grpname);
            }
            fprintf(fp, "\n");
            printsc = true;
        }

        if (!printsc) {
            CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(),
                Electrical);
            if (esdesc) {
                CDp_snode **nary;
                unsigned int nsize = esdesc->checkTerminals(&nary);
                if (nsize) {
                    // The topsdesc has terminals, add a .subckt line,
                    // using the electrical ordering.

                    fprintf(fp, ".subckt %s",
                        Tstring(gd_celldesc->cellname()));
                    for (unsigned int i = 0; i < nsize; i++) {
                        if (!nary[i])
                            continue;
                        CDsterm *term = nary[i]->cell_terminal();
                        if (!term)
                            continue;

                        int grp = -1;
                        for (int j = 1; j < gd_asize; j++) {
                            CDpin *p = gd_groups[j].termlist();
                            for ( ; p; p = p->next()) {
                                if (p->term() == term) {
                                    grp = j;
                                    break;
                                }
                            }
                            if (p)
                                break;
                        }
                        if (grp >= 0) {
                            const char *grpname = group_name(grp, &lstr);
                            fprintf(fp, " %s", grpname);
                        }
                    }
                    delete [] nary;
                    printsc = true;
                    fprintf(fp, "\n");
                }
            }
        }
        if (!printsc) {
            // No electrical node property, get the list from the
            // XICP_TERM_ORDER property, if it exists.  The names had
            // better match net names, those that don't are silently
            // ignored.

            CDp *pto = gd_celldesc->prpty(XICP_TERM_ORDER);
            if (pto) {
                fprintf(fp, ".subckt %s", Tstring(gd_celldesc->cellname()));
                const char *s = pto->string();
                char *tok;
                while ((tok = lstring::gettok(&s)) != 0) {
                    CDnetName nm = CDnetex::name_tab_add(tok);
                    delete [] tok;
                    for (int i = 1; i < gd_asize; i++) {
                        if (gd_groups[i].netname() == nm) {
                            if (opts->isset(opt_atom_labels)) {
                                fprintf(fp, " %d", i);
                                sprintf(buf, "* %d %s\n", i, Tstring(nm));
                                lstr.add(buf);
                            }
                            else
                                fprintf(fp, " %s", Tstring(nm));
                        }
                    }
                }
                printsc = true;
                fprintf(fp, "\n");
            }
        }
    }
    else {
        sSubcDesc *scd = EX()->findSubcircuit(gd_celldesc);
        if (scd) {
            fprintf(fp, ".subckt %s", Tstring(gd_celldesc->cellname()));
            for (unsigned int i = 0; i < scd->num_contacts(); i++) {
                int grp = scd->contact(i);
                const char *grpname = group_name(grp, &lstr);
                fprintf(fp, " %s", grpname);
            }
            fprintf(fp, "\n");
            printsc = true;
        }
        if (!printsc) {
            // Add a "bogus" .subckt line.

            fprintf(fp, ".subckt %s\n", Tstring(gd_celldesc->cellname()));
            printsc = true;
        }
    }
    if (lstr.string())
        fprintf(fp, "%s", lstr.string());

    // Print devices,  First pass: set the per-prefix index number.
    //
    SymTab *name_tab = new SymTab(false, false);
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                const char *pf = di->desc()->prefix();
                if (pf) {
                    SymTabEnt *ent = SymTab::get_ent(name_tab, pf);
                    if (!ent) {
                        name_tab->add(pf, 0, false);
                        ent = SymTab::get_ent(name_tab, pf);
                    }
                    unsigned long n = (unsigned long)ent->stData;
                    di->set_spindex(n);
                    n++;
                    ent->stData = (void*)n;
                }
            }
        }
    }
    delete name_tab;

    // Second pass: write the device line, using the prefix index.
    // Names referenced will be correct due to the two passes.
    //
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *di = p->devs(); di; di = di->next()) {
                if (!di->print_net(fp, opts))
                    return (false);
            }
        }
    }

    if (opts->isset(opt_atom_cap)) {
        // print wire caps
        bool msg_p = false;
        for (int i = 1; i < gd_asize; i++) {
            if (gd_groups[i].capac() != 0.0 && (gd_groups[i].termlist() ||
                    gd_groups[i].device_contacts() ||
                    gd_groups[i].subc_contacts())) {
                if (!msg_p) {
                    fprintf(fp, "** Wire net capacitances\n");
                    msg_p = true;
                }
                // The capac() is in picofarads.
                fprintf(fp, "%s%d %d 0 %s\n", WIRECAP_PREFIX, i, i,
                    SPnum.printnum(1e-12*gd_groups[i].capac(), "F"));
            }
        }
    }

    // print subckts
    if (EX()->skipExtract(gd_celldesc) && gd_celldesc != topsdesc) {
        // The cell is empty, but (presumably) the .subckt line
        // has been printed.  Add comment lines indicating the
        // terminal names.

        sSubcDesc *scd = EX()->findSubcircuit(gd_celldesc);
        if (scd) {
            for (unsigned int i = 0; i < scd->num_contacts(); i++) {
                int grp = scd->contact(i);
                if (grp >= 0 && grp < gd_asize) {
                    CDpin *p = gd_groups[grp].termlist();
                    // should be a formal term
                    if (p) {
                        fprintf(fp, "* %d %s\n", grp,
                            Tstring(p->term()->name()));
                    }
                }
                else
                    fprintf(fp, "* %d %s\n", grp, "???");
            }
        }
    }
    else {
        int opens = 0;
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            for (sSubcInst *s = sl->subs(); s; s = s->next()) {

                CDp *pd = s->cdesc()->prpty(XICP_INST);
                if (pd && !opts->spice_intern_instname() &&
                        pd->string() && *pd->string()) {
                    if (*pd->string() == 'x' || *pd->string() == 'X')
                        fprintf(fp, "%s", pd->string());
                    else
                        fprintf(fp, "X_%s", pd->string());
                }
                else {
                    if (opts->spice_print_mode() != PSPM_physical) {
                        if (s->dual()) {
                            char *instname = s->dual()->instance_name();
                            fprintf(fp, "%s", instname);
                            delete [] instname;
                        }
                        else if (opts->spice_print_mode() == PSPM_mixed) {
                            fprintf(fp, "X_%d", s->uid());
                        }
                        else
                            continue;
                    }
                    else
                        fprintf(fp, "X_%d", s->uid());
                }
                sSubcDesc *scd =
                    EX()->findSubcircuit(s->cdesc()->masterCell(true));
                bool error = false;
                if (scd) {
                    unsigned int ccnt = 0;
                    for (sSubcContactInst *c = s->contacts(); c; c = c->next())
                        ccnt++;
                    if (ccnt != scd->num_contacts())
                        error = true;
                    for (unsigned int i = 0; i < scd->num_contacts(); i++) {
                        bool found = false;
                        for (sSubcContactInst *c = s->contacts(); c;
                                c = c->next()) {
                            if (c->subc_group() == scd->contact(i)) {
                                fprintf(fp, " %s",
                                    group_name(c->parent_group(), 0));
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            fprintf(fp, " nc%d", opens++);
                            error = true;
                        }
                    }
                }
                else {
                    for (sSubcContactInst *c = s->contacts(); c; c = c->next())
                        fprintf(fp, " %s", group_name(c->parent_group(), 0));
                    if (s->contacts())
                        error = true;
                }
                fprintf(fp, " %s\n", Tstring(s->cdesc()->cellname()));

                if (error) {
                    fprintf(fp, "  *** contact list doesn't match template\n");
                    fprintf(fp, "* contacts:");
                    for (sSubcContactInst *c = s->contacts(); c; c = c->next())
                        fprintf(fp, " %d", c->subc_group());
                    fprintf(fp, "\n");
                    fprintf(fp, "* template:");
                    if (scd) {
                        for (unsigned int i = 0; i < scd->num_contacts(); i++)
                            fprintf(fp, " %d", scd->contact(i));
                        fprintf(fp, "\n");
                    }
                    else
                        fprintf(fp, " not found\n");
                }
            }
        }
    }

    if (printsc)
        fprintf(fp, ".ends %s\n", Tstring(gd_celldesc->cellname()));
    fprintf(fp, "\n");
    return (true);
}


void
cGroupDesc::print_formatted(FILE *fp, const char *format, sDumpOpts *opts)
{
    if (!format || !*format)
        return;

    // find script for format
    SymTab *format_tab = XM()->GetFormatFuncTab(EX_PNET_FORMAT);
    if (!format_tab)
        return;
    SIfunc *sf = (SIfunc*)SymTab::get(format_tab, format);
    if (!sf || sf == (SIfunc*)ST_NIL) {
        // not found
        return;
    }

    // set variables
    siVariable *variables = XM()->GetFormatVars(EX_PNET_FORMAT);
    for (Variable *v = variables; v; v = v->next) {
        if (!v->name)
            continue;
        if (!strcmp(v->name, "_cellname"))
            v->content.string = (char*)gd_celldesc->cellname();
        else if (!strcmp(v->name, "_viewname"))
            v->content.string = (char*)"physical";
        else if (!strcmp(v->name, "_techname"))
            v->content.string = (char*)(Tech()->TechnologyName() ?
                Tech()->TechnologyName() : TECH_DEFAULT);
        else if (!strcmp(v->name, "_num_nets"))
            v->content.value = gd_asize;
        else if (!strcmp(v->name, "_mode"))
            v->content.value = Physical;
        else if (!strcmp(v->name, "_list_all"))
            v->content.value = opts->isset(opt_atom_all);
        else if (!strcmp(v->name, "_bottom_up"))
            v->content.value = opts->isset(opt_atom_btmup);
        else if (!strcmp(v->name, "_show_geom"))
            v->content.value = opts->isset(opt_atom_geom);
        else if (!strcmp(v->name, "_show_wire_cap"))
            v->content.value = opts->isset(opt_atom_cap);
        else if (!strcmp(v->name, "_ignore_labels"))
            v->content.value = opts->isset(opt_atom_labels);
    }
    siVariable *tv = SIparse()->getVariables();
    SIparse()->setVariables(variables);

    // redirect stdout to fp
    fflush(fp);
    int so = fileno(stdout);
    int n = dup(so);
    dup2(fileno(fp), so);

    // exec script
    CDcbin cbin(gd_celldesc);
    CDcellName tmp_cellname = DSP()->CurCellName();
    CDcellName tmp_topname = DSP()->TopCellName();
    DSP()->SetCurCellName(cbin.cellname());
    DSP()->SetTopCellName(cbin.cellname());
    SIlexprCx cx;
    SI()->EvalFunc(sf, &cx);
    DSP()->SetCurCellName(tmp_cellname);
    DSP()->SetTopCellName(tmp_topname);

    // revert stdout
    fflush(stdout);
    dup2(n, so);
    close(n);

    SIparse()->setVariables(tv);
}

