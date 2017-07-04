
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2013 Whiteley Research Inc, all rights reserved.        *
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
 $Id: ext_out_elec.cc,v 5.10 2017/03/14 01:26:38 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_nets.h"
#include "sced.h"
#include "cd_celldb.h"
#include "cd_netname.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "tech.h"
#include "errorlog.h"

/*========================================================================*
 *
 *  Functions for dumping schematic netlist output.
 *
 *========================================================================*/


// Main function to dump netlist information extracted from the
// schematic.
//
bool
cExt::dumpElecNetlist(FILE *fp, CDs *sdesc, sDumpOpts *opts)
{
    if (!sdesc)
        return (true);
    if (!sdesc->isElectrical()) {
        sdesc = CDcdb()->findCell(sdesc->cellname(), Electrical);
        if (!sdesc)
            return (true);
    }

    dspPkgIf()->SetWorking(true);
    XM()->OpenFormatLib(EX_ENET_FORMAT);
    SymTab *tab = new SymTab(false, false);
    bool ret = false;
    if (!skipExtract(sdesc)) {
        bool format_given = opts->isset(opt_atom_net) ||
            opts->isset(opt_atom_spice);
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
            ret = dump_elec_recurse(fp, sdesc, opts->depth(), opts, tab,
                0);
    }
    delete tab;
    dspPkgIf()->SetWorking(false);
    return (ret);
}


namespace {
    void
    pr_line(FILE *fp, sp_line_t *d0)
    {
        for (sp_line_t *d = d0; d; d = d->li_next) {
#ifdef SP_LINE_FULL
            if (d->li_actual != 0) {
                for (sp_line_t *d1 = d->li_actual; d1; d1 = d1->li_next)
                    fprintf(fp, "%s\n", d1->li_line);
            }
            else
#endif
            fprintf(fp, "%s\n", d->li_line);
        }
        if (d0)
            fprintf(fp, "\n");
    }
}


// Recursive private core of DumpCell() for electrical cells.
//
bool
cExt::dump_elec_recurse(FILE *fp, CDs *sdesc, int depth, sDumpOpts *opts,
    SymTab *tab, SymTab *sptab)
{
    bool spice_only = false;
    if (depth == opts->depth() && opts->isset(opt_atom_spice)) {
        for (int i = 0; i < opts->num_buttons(); i++) {
            if (opts->button(i)->is_set()) {
                if (opts->button(i)->atom() == opt_atom_spice)
                    spice_only = true;
                else if (opts->button(i)->atom() != opt_atom_btmup) {
                    spice_only = false;
                    break;
                }
            }
        }
        // Print title line
        sp_line_t *d0 = SCD()->makeSpiceDeck(sdesc, &sptab);
        if (spice_only)
            pr_line(fp, d0);
        sp_line_t::destroy(d0);
    }

    tab->add((unsigned long)sdesc, 0, false);
    if (!opts->isset(opt_atom_btmup)) {
        if (!dump_elec(fp, sdesc, depth, opts, tab, sptab))
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
            if (tab->get((unsigned long)msdesc) != ST_NIL)
                continue;
            if (skipExtract(msdesc))
                continue;
            if (!dump_elec_recurse(fp, msdesc, depth-1, opts, tab, sptab))
                return (false);
        }
    }
    if (opts->isset(opt_atom_btmup)) {
        if (!dump_elec(fp, sdesc, depth, opts, tab, sptab))
            return (false);
    }

    // clear spice table
    if (depth == opts->depth() && sptab) {
        SymTabGen gen(sptab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete [] h->stTag;
            sp_line_t::destroy((sp_line_t*)h->stData);
            h->stData = 0;
            h->stTag = 0;
            delete h;
        }
        delete sptab;
    }
    return (true);
}


// Private call to actually dump the electrical netlist.
//
bool
cExt::dump_elec(FILE *fp, CDs *sdesc, int, sDumpOpts *opts, SymTab*,
    SymTab *sptab)
{
    if (opts->isset(opt_atom_net)) {
        fprintf(fp, "############  %s\n", XM()->IdString());
        fprintf(fp, "Electrical netlist of cell %s\n",
            sdesc->cellname()->string());
        sElecNetList *eterms = new sElecNetList(sdesc);
        if (!eterms) {
            fprintf(fp, "No electrical nets found.\n\n");
            return (true);
        }
        int size = eterms->size();
        for (int i = 0; i < size; i++) {
            if (eterms->node_active(i)) {
                if (i == 0)
                    fprintf(fp, "GND:\n");
                else {
                    const char *nn = SCD()->nodeName(sdesc, i);
                    if (isdigit(*nn))
                        fprintf(fp, "Net%s:\n", nn);
                    else
                        fprintf(fp, "%s:\n", nn);
                }

                for (CDpin *p = eterms->pins_of_node(i); p; p = p->next())
                    fprintf(fp, "\t%s\n", p->term()->name()->string());
                for (CDcont *t = eterms->conts_of_node(i); t; t = t->next())
                    fprintf(fp, "\t%s\n", t->term()->name()->string());
            }
        }
        delete eterms;
        fprintf(fp, "\n");
    }
    if (opts->isset(opt_atom_spice)) {
        sp_line_t *d0;
        d0 = sptab ? (sp_line_t*)sptab->get(sdesc->cellname()->string()) : 0;
        if (d0 == (sp_line_t*)ST_NIL)
            d0 = 0;
        pr_line(fp, d0);
    }
    for (int i = opts->user_button(); i < opts->num_buttons(); i++) {
        if (opts->button(i)->is_set())
            dump_elec_formatted(fp, sdesc, opts->button(i)->name());
    }
    return (true);
}


// Private call to dump electrical netlist in user's format.
//
bool
cExt::dump_elec_formatted(FILE *fp, CDs *sdesc, const char *format)
{
    if (!format || !*format)
        return (false);

    // find script for format
    SymTab *format_tab = XM()->GetFormatFuncTab(EX_ENET_FORMAT);
    if (!format_tab)
        return (false);
    SIfunc *sf = (SIfunc*)format_tab->get(format);
    if (!sf || sf == (SIfunc*)ST_NIL) {
        // not found
        return (false);
    }

    // set variables
    siVariable *variables = XM()->GetFormatVars(EX_ENET_FORMAT);
    for (siVariable *v = variables; v; v = (siVariable*)v->next) {
        if (!v->name)
            continue;
        if (!strcmp(v->name, "_cellname"))
            v->content.string = (char*)sdesc->cellname();
        else if (!strcmp(v->name, "_viewname"))
            v->content.string = (char*)"physical";
        else if (!strcmp(v->name, "_techname"))
            v->content.string = (char*)(Tech()->TechnologyName() ?
                Tech()->TechnologyName() : TECH_DEFAULT);
        else if (!strcmp(v->name, "_num_nets"))
            v->content.value = 0;
        else if (!strcmp(v->name, "_mode"))
            v->content.value = Electrical;
    }
    siVariable *tv = SIparse()->getVariables();
    SIparse()->setVariables(variables);

    // redirect stdout to fp
    fflush(fp);
    int so = fileno(stdout);
    int n = dup(so);
    dup2(fileno(fp), so);

    // exec script
    CDcbin cbin(sdesc);
    CDcellName tmp_cellname = DSP()->CurCellName();
    CDcellName tmp_topname = DSP()->TopCellName();
    DSP()->SetCurCellName(cbin.cellname());
    DSP()->SetTopCellName(cbin.cellname());
    SIlexprCx cx;
    XIrt ret = SI()->EvalFunc(sf, &cx);
    DSP()->SetCurCellName(tmp_cellname);
    DSP()->SetTopCellName(tmp_topname);

    // revert stdout
    fflush(stdout);
    dup2(n, so);
    close(n);

    SIparse()->setVariables(tv);
    return (ret == XIok);
}

