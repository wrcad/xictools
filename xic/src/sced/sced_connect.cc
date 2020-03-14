
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
#include "sced.h"
#include "extif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "cd_netname.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "errorlog.h"
#include "sced_connect.h"
#include "sced_nodemap.h"
#include "sced_errlog.h"
#include "pcell_params.h"
#include "miscutil/symtab.h"


/*========================================================================*
 *
 *  Functions for establishing electrical database connectivity
 *
 *========================================================================*/

#define INIT_NTSIZE 200

// Encapsulation of connectivity establishment functions.
//

namespace {

    /*** Debugging
    void pr_nodes(const CDs *sdesc)
    {
        CDg gdesc;
        gdesc.init_gen(sdesc, CellLayer());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
            CDgenRange rgen(pr);
            int vix = 0;
            while (rgen.next(0)) {
                CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pc; pc = pc->next()) {
                    CDp_cnode *px;
                    if (vix > 0)
                        px = pr->node(0, vix, pc->index());
                    else
                        px = pc;
                    if (px)
                        printf("%s %d\n", px->term_name(), px->enode());
                }
                vix++;
            }
        }
    }
    ***/

    bool has_shorted_nophys(CDs *sd)
    {
        CDg gdesc;
        gdesc.init_gen(sd, CellLayer());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDp *pd = cdesc->prpty(P_NOPHYS);
            if (pd && pd->string() &&
                    (*pd->string() == 's' || *pd->string() == 'S'))
                return (true);
        }
        return (false);
    }


    bool connect_recurse(CDs *sd, int dcnt)
    {
        if (!sd)
            return (true);
        if (!sd->isElectrical())
            return (false);
        CDs *tsd = sd->owner();
        if (tsd)
            sd = tsd;
        if (sd->cellname() == DSP()->CurCellName())
            dspPkgIf()->SetWorking(true);
        CDm_gen mgen(sd, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            if (!mdesc->hasInstances())
                continue;
            CDs *msdesc = mdesc->celldesc();
            if (msdesc->isDevice() || ExtIf()->skipExtract(msdesc))
                continue;
            if (!connect_recurse(msdesc, dcnt + 1)) {
                dcnt = 1000;
                break;
            }
        }
        bool ret = true;
        if (dcnt > 50)
            ret = false;
        else 
            ret = SCD()->connect(sd);
        if (sd->cellname() == DSP()->CurCellName())
            dspPkgIf()->SetWorking(false);
        return (ret);
    }
}


// This function establishes the circuit connectivity for the current
// hierarchy.  If for_spice is false, the NOPHYS devices are ignored,
// and the netlist will have the "shorted" NOPHYS devices shorted out. 
// This is appropriate for LVS and other extraction system operations.
//
// If for_spice is true, the NOPHYS devices are included, not shorted. 
// This applies when generating output for SPICE simulation.
//
// If there are no shorted NOPHYS devices, for_spice has no effect.
//
bool
cSced::connectAll(bool for_spice, CDs *sd)
{
    if (sd) {
        if (!sd->isElectrical())
            return (false);
        CDs *tsd = sd->owner();
        if (tsd)
            sd = tsd;
    }
    else
        sd = CurCell(Electrical, true);
    if (!sd)
        return (false);

    bool last_np = sc_include_nophys;
    sc_include_nophys = for_spice;

    ScedErrLog.start_logging(0);
    dspPkgIf()->SetWorking(true);
    bool ret = connect_recurse(sd, 0);
    dspPkgIf()->SetWorking(false);
    ScedErrLog.end_logging();

    // If this was not the current cell, reset the sc_include_nophys
    // flag to the previous state.
    if (sd->cellname() != DSP()->CurCellName())
        sc_include_nophys = last_np;
    return (ret);
}


// Call this is something changes that would invalidate all
// connectivity.
//
void
cSced::unconnectAll()
{
    CDcbin cbin;
    CDgenTab_cbin gen;
    while (gen.next(&cbin)) {
        if (cbin.elec())
            cbin.elec()->unsetConnected();
        if (cbin.phys())
            cbin.phys()->unsetConnected();
    }
    const char *tabname = CDcdb()->tableName();
    CDcdb()->rotateTable();
    while (CDcdb()->tableName() != tabname) {
        gen = CDgenTab_cbin();
        while (gen.next(&cbin)) {
            if (cbin.elec())
                cbin.elec()->unsetConnected();
            if (cbin.phys())
                cbin.phys()->unsetConnected();
        }
    }
}


// Assign node numbers to the terminals and wires.
// This also takes care of updating all properties and labels, and the
// screen.
//
// The connect function is OK to call in physical mode, argument
// must be an electrical desc.
//
// If sc_include_nophys is true, connectivity is established with
// devices that have the "shorted" NOPHYS property are taken as
// ordinary devices.  These are parasitic elements in the schematic
// that are included in simulation netlists but not compared in LVS.
//
bool
cSced::connect(CDs *sd)
{
    static bool lock;

    if (lock)
        return (true);
    if (!sd)
        return (true);
    if (!sd->isElectrical())
        return (false);
    CDs *tsd = sd->owner();
    if (tsd)
        sd = tsd;
    sd->hyInit();

    // Simplify wires.
    if (sd->cellname() == DSP()->CurCellName())
        fixPaths(sd);

    if (sd->isSPconnected() && !sd->isConnected())
        // Shouldn't happen.
        sd->setSPconnected(false);

    bool nm_dirty = sd->nodes() ? sd->nodes()->isDirty() : false;
    if (sc_include_nophys && !sd->isSPconnected()) {
        bool has_snp = has_shorted_nophys(sd);
        if (!sd->isConnected() || has_snp) {
            cScedConnect cx;
            if (sd->cellname() == DSP()->CurCellName())
                dspPkgIf()->SetWorking(true);
            cx.run(sd, false);
            if (sd->cellname() == DSP()->CurCellName())
                dspPkgIf()->SetWorking(false);
            nm_dirty = true;
        }
        sd->setSPconnected(true);
        sd->setConnected(!has_snp);
    }
    else if (!sc_include_nophys && !sd->isConnected()) {
        bool has_snp = has_shorted_nophys(sd);
        if (!sd->isSPconnected() || has_snp) {
            cScedConnect cx;
            if (sd->cellname() == DSP()->CurCellName())
                dspPkgIf()->SetWorking(true);
            cx.run(sd, true);
            if (sd->cellname() == DSP()->CurCellName())
                dspPkgIf()->SetWorking(false);
            nm_dirty = true;
        }
        sd->setConnected(true);
        sd->setSPconnected(!has_snp);
    }

    // The node map will exist if connect was run.
    if (nm_dirty) {
        // This will call connect again.  If all is right, nm_dirty
        // will not be set on the recursive calls so we won' get here
        // again.  To be sure, the static lock is also used.

        lock = true;
        cNodeMap *map = sd->nodes();
        if (map) {
            setModified(map);
            map->updateProperty();
            if (sd->cellname() == DSP()->CurCellName())
                PopUpNodeMap(0, MODE_UPD);
        }
        lock = false;
    }

    return (true);
}


// Redraw all of the unassociated hypertext labels.  Do this after an
// update.
//
void
cSced::updateHlabels(CDs *sd)
{
    if (!sd || !sd->isElectrical())
        return;
    if (sd->cellname() != DSP()->CurCellName())
        return;
    CDs *tsd = sd->owner();
    if (tsd)
        sd = tsd;

    CDl *ldesc;
    CDsLgen lgen(sd);
    while ((ldesc = lgen.next()) != 0) {
        if (ldesc->index(Electrical) < 2)
            // skip SCED layer
            continue;
        CDg gdesc;
        gdesc.init_gen(sd, ldesc);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->type() != CDLABEL)
                continue;
            CDp_lref *prf = (CDp_lref*)odesc->prpty(P_LABRF);
            if (prf && prf->devref())
                // already redrawn
                continue;
            CDla *olabel = (CDla*)odesc;
            hyList *hlabel = olabel->label();
            while (hlabel) {
                if (hlabel->ref_type() != HLrefText)
                    break;
                hlabel = hlabel->next();
            }
            if (!hlabel)
                continue;

            // Found a hypertext link.  Redraw, link might have
            // changed.
            //
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsSimilar(Electrical, DSP()->MainWdesc()))
                    wdesc->Redisplay(&odesc->oBB());
            }
        }
    }
}


// Update the internal names of all devices and mutual inductors. 
// Takes care of everything, including screen refresh.  This is called
// after an operation, and is only needed if the current connectivity
// needs updating.
//
void
cSced::updateNames(CDs *sd)
{
    if (!sd || !sd->isElectrical())
        return;
    CDs *tsd = sd->owner();
    if (tsd)
        sd = tsd;

    // Check connectivity status.
    bool nm_dirty = false;
    if (sc_include_nophys && !sd->isSPconnected()) {
        bool has_snp = has_shorted_nophys(sd);
        if (!sd->isConnected() || has_snp)
            nm_dirty = true;
    }
    else if (!sc_include_nophys && !sd->isConnected()) {
        bool has_snp = has_shorted_nophys(sd);
        if (!sd->isSPconnected() || has_snp)
            nm_dirty = true;
    }
    if (!nm_dirty)
        return;

    renumberInstances(sd);

    sd->updateTermNames(CDs::UTNinstances);
    sd->prptyMutualUpdate();
}


void
cSced::renumberInstances(CDs *sd)
{
    if (!sd || !sd->isElectrical())
        return;
    CDs *tsd = sd->owner();
    if (tsd)
        sd = tsd;

    SymTab *stab = new SymTab(false, false);
    SymTab *scstab = new SymTab(false, false);

    // In order to number devices that share a letter correctly, e.g.,
    // 'q' for 'npn' and 'pnp', save the reference counts in a
    // name-accessed symbol table.
    // Devices are numbered in database order (top-down, left to right
    // of UL BB corner).

    CDg gdesc;
    gdesc.init_gen(sd, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            continue;
        CDp_sname *pna = (CDp_sname*)msdesc->prpty(P_NAME);
        if (!pna || !pna->name_string())
            continue;
 
        long dcnt = (long)SymTab::get(stab, (unsigned long)pna->name_string());
        if (dcnt == (long)ST_NIL)
            dcnt = 0;
        stab->replace((unsigned long)pna->name_string(), (void*)(dcnt+1));
        cdesc->updateDeviceName(dcnt);

        // Subcircuits have an additional ordering number, which is
        // zero-based for each subcircuit master.  We have already
        // computed the absolute ordering number.

        if (pna->is_subckt() && !pna->is_macro()) {
            dcnt = (long)SymTab::get(scstab, (unsigned long)cdesc->cellname());
            if (dcnt == (long)ST_NIL)
                dcnt = 0;
            scstab->replace((unsigned long)cdesc->cellname(), (void*)(dcnt+1));
            CDp_cname *pnc = (CDp_cname*)cdesc->prpty(P_NAME);
            if (pnc)
                pnc->set_scindex(dcnt);
        }
    }
    delete stab;
    delete scstab;
}
// End of cSced functions


namespace {
    // Return the cell node property with given index, or 0 if not
    // found.
    //
    CDp_snode *find_node_prp(const CDs *sdesc, const CDp_bsnode *pb,
        unsigned int indx)
    {
        if (pb->index_map())
            indx = pb->index_map()[indx];
        else
            indx += pb->index();
        if (indx <= P_NODE_MAX_INDEX) {
            CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                if (pn->index() == indx)
                    return (pn);
            }
        }
        return (0);
    }


    // Return the instance node property with given index, or 0 if not
    // found.
    //
    CDp_cnode *find_node_prp(const CDc *cdesc, const CDp_bcnode *pb,
        unsigned int indx)
    {
        if (pb->index_map())
            indx = pb->index_map()[indx];
        else
            indx += pb->index();
        if (indx <= P_NODE_MAX_INDEX) {
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                if (pn->index() == indx)
                    return (pn);
            }
        }
        return (0);
    }


    // Return true if pcn is connectable for the issym mode and
    // connected at xloc, yloc.
    //
    bool in_contact(const CDp_cnode *pcn, bool issym, int xloc, int yloc)
    {
        if (pcn->has_flag(TE_BYNAME))
            return (false);
        if (issym) {
            if (pcn->has_flag(TE_SYINVIS))
                return (false);
        }
        else {
            if (pcn->has_flag(TE_SCINVIS))
                return (false);
        }
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (!pcn->get_pos(ix, &x, &y))
                break;
            if (xloc == x && yloc == y)
                return (true);
        }
        return (false);
    }


    // Return true if pcr is connectable for the issym mode and
    // connected at xloc, yloc.
    //
    bool in_contact(const CDp_bcnode *pcb, bool issym, int xloc, int yloc)
    {
        if (issym) {
            if (pcb->has_flag(TE_SYINVIS))
                return (false);
        }
        else {
            if (pcb->has_flag(TE_SCINVIS))
                return (false);
        }
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (!pcb->get_pos(ix, &x, &y))
                break;
            if (xloc == x && yloc == y)
                return (true);
        }
        return (false);
    }
}


cScedConnect::~cScedConnect()
{
    for (int i = 0; i < cn_count; i++)
        node_list::destroy(cn_ntab[i]);
    delete [] cn_ntab;

    if (cn_case_insens)
        delete cn_u.tname_tab_ci;
    else
        delete cn_u.tname_tab;

    for (CDol *ol = cn_tmp_wires; ol; ol = cn_tmp_wires) {
        cn_tmp_wires = ol->next;
        delete (CDw*)ol->odesc;
        delete ol;
    }
    CDp::destroy(cn_ndprps);
    CDp::destroy(cn_btprps);

    delete cn_wire_tab;
    while (cn_wire_stack) {
        cstk_elt *x = cn_wire_stack;
        cn_wire_stack = cn_wire_stack->next();
        delete x;
    }
}


// Public function to run connectivity engine.
//
void
cScedConnect::run(CDs *sd, bool lvsmode)
{
    if (!sd)
        return;
    // Only process real subcircuits, not devices, macros, terminals.
    {
        CDelecCellType tp = sd->elecCellType();
        if (tp != CDelecSubc) {
            if (tp != CDelecNull)
                return;

            // We're not a subckt, but that's perfectly ok if we just
            // haven't added any name/node properties at the top level. 
            // If we have an instance of something, we will proceed.

            bool ok = false;
            CDm_gen mgen(sd, GEN_MASTERS);
            for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
                if (mdesc->hasInstances()) {
                    ok = true;
                    break;
                }
            }
            if (!ok)
                return;
        }
    }
    ScedErrLog.start_logging(sd->cellname());

    // Add a name table if not already present.
    if (!sd->nodes())
        sd->setNodes(new cNodeMap(sd));

    init(sd, lvsmode);
    connect();
    ScedErrLog.end_logging();

    SymTab *ntab = new SymTab(false, false);
    if (cn_case_insens) {
        tgen_t<name_elt> gen(cn_u.tname_tab_ci);
        name_elt *ne;
        while ((ne = gen.next()) != 0) {
            if (ne->index() >= 0) {
                CDnetName nm = CDnetex::mk_name(Tstring(ne->name()),
                    ne->index());
                ntab->add((unsigned long)nm,
                    (void*)(long)ne->nodeprp()->enode(), false);
            }
            else
                ntab->add((unsigned long)ne->name(),
                    (void*)(long)ne->nodeprp()->enode(), false);
        }
    }
    else {
        tgen_t<name_elt> gen(cn_u.tname_tab);
        name_elt *ne;
        while ((ne = gen.next()) != 0) {
            if (ne->index() >= 0) {
                CDnetName nm = CDnetex::mk_name(Tstring(ne->name()),
                    ne->index());
                ntab->add((unsigned long)nm,
                    (void*)(long)ne->nodeprp()->enode(), false);
            }
            else
                ntab->add((unsigned long)ne->name(),
                    (void*)(long)ne->nodeprp()->enode(), false);
        }
    }
    // Pass the number of nodes and the name table to the node name map.
    sd->nodes()->setupNetNames(cn_count, ntab);

    // Update mutual inductors, terminal names, and property labels.
    sd->prptyMutualUpdate();
    sd->updateTermNames(CDs::UTNterms);
    SCD()->updateHlabels(sd);
}


// Initialize and set up for connectivity assignments.
//
void
cScedConnect::init(CDs *sd, bool lvsmode)
{
    // Make sure to get the schematic cell if passed a symbolic
    // cell.
    CDs *tsd = sd->owner();
    cn_sdesc = tsd ? tsd : sd;

    new_node();
    cn_case_insens = CDnetex::name_tab_case_insens();
    if (cn_case_insens)
        cn_u.tname_tab_ci = new csntable_t<name_elt>;
    else
        cn_u.tname_tab = new sntable_t<name_elt>;

    // Initialize all scalar connection node numbers to -1.
    //
    for (CDp_snode *pn = (CDp_snode*)cn_sdesc->prpty(P_NODE); pn;
            pn = pn->next()) {
        pn->set_enode(-1);
    }

    // Check and set up the node index map if necessary.  Reflect
    // changes to instance terminals.
    //
    for (CDp_bsnode *ps = (CDp_bsnode*)cn_sdesc->prpty(P_BNODE); ps;
            ps = ps->next()) {
        setup_map(ps);
    }
    cn_sdesc->reflectTerminals();

    // Initialize cell connection bus terminals.
    //
    for (CDp_bsnode *ps = (CDp_bsnode*)cn_sdesc->prpty(P_BNODE); ps;
            ps = ps->next()) {

        if (!ps->has_name())
            continue;

        CDnetexGen ngen(ps);
        unsigned int indx = 0;
        CDnetName nm;
        int n;
        while (ngen.next(&nm, &n)) {
            if (nm) {
                CDp_snode *px = find_node_prp(cn_sdesc, ps, indx);
                if (px) {
                    name_elt *ne = tname_tab_find(Tstring(nm), n);
                    if (ne)
                        add_to_ntab(ne->nodenum(), px);
                    else {
                        ne = cn_name_elt_alloc.new_element();
                        ne->set(cn_count, nm, n, px);
                        add_to_tname_tab(ne);
                        add_to_ntab(cn_count, px);
                        new_node();
                    }
                }
            }
            else {
                char *tn = ps->id_text();
                ScedErrLog.add_err("cell terminal has invalid name %s.", tn);
                delete [] tn;
            }
            indx++;
        }
    }

    // Process the named scalar terminals.
    //
    for (CDp_snode *pn = (CDp_snode*)cn_sdesc->prpty(P_NODE); pn;
            pn = pn->next()) {
        if (!pn->get_term_name())
            continue;

        CDnetName nm;
        int n;
        if (!CDnetex::parse_bit(Tstring(pn->get_term_name()), &nm, &n)) {
            ScedErrLog.add_err("cell terminal has invalid name %s.\n%s",
                pn->term_name(), Errs()->get_error());
            continue;
        }
        if (pn->enode() < 0) {
            name_elt *ne = tname_tab_find(Tstring(nm), n);
            if (ne)
                add_to_ntab(ne->nodenum(), pn);
            else {
                ne = cn_name_elt_alloc.new_element();
                ne->set(cn_count, nm, n, pn);
                add_to_tname_tab(ne);
                add_to_ntab(cn_count, pn);
                new_node();
            }
        }
        else {
            name_elt *ne = tname_tab_find(Tstring(nm), n);
            if (ne) {
                if (ne->nodenum() != pn->enode()) {
                    // This indicates a name clash in the terminal
                    // naming, an error.

                    ScedErrLog.add_err("terminal name clash involving %s.",
                        pn->term_name());
                }
            }
            else {
                // This scalar terminal now has (at least) two names.

                ne = cn_name_elt_alloc.new_element();
                ne->set(pn->enode(), nm, n, pn);
                add_to_tname_tab(ne);
            }
        }
    }

    // Create a list of dummy node properties from the node mapping
    // assignments.  These will behave like the CDelecTerm devices in
    // enforcing connectivity between (sub)nets.
    //
    CDp::destroy(cn_ndprps);
    cn_ndprps = 0;
    CDp_cnode *pe = 0;
    xyname_t *setnames = 0;
    if (cn_sdesc->nodes())
        setnames = cn_sdesc->nodes()->getSetList();
    for (xyname_t *xy = setnames; xy; xy = xy->next()) {
        CDp_cnode *pc = new CDp_cnode;
        ScedErrLog.add_log("nodemap %s", xy->name());
        pc->set_string(xy->name());
        pc->set_pos(0, xy->posx(), xy->posy());
        pc->set_enode(-1);
        if (!cn_ndprps)
            cn_ndprps = pe = pc;
        else {
            pe->set_next_prp(pc);
            pe = pc;
        }
    }
    xyname_t::destroy(setnames);

    // Initialize the dummy node-mapping property nodes.
    //
    for (CDp_cnode *pn = cn_ndprps; pn; pn = (CDp_cnode*)pn->next_prp()) {

        CDnetName nm;
        int n;
        if (!CDnetex::parse_bit(pn->string(), &nm, &n)) {
            ScedErrLog.add_err("bad assigned net name %s, ignored.\n%s",
                pn->string(), Errs()->get_error());
            continue;
        }
        name_elt *ne = tname_tab_find(Tstring(nm), n);
        if (ne)
            add_to_ntab(ne->nodenum(), pn);
        else {
            ne = cn_name_elt_alloc.new_element();
            ne->set(cn_count, nm, n, pn);
            add_to_tname_tab(ne);
            add_to_ntab(cn_count, pn);
            ScedErrLog.add_log("new node %s %d", pn->string(), pn->enode());
            new_node();
        }
    }

    // These are dummy node properties used for named bus terminals
    // and named bus terminal devices.
    //
    CDp::destroy(cn_btprps);
    cn_btprps = 0;

    // Add a dummy ground node.  The name "0" will match node number
    // 0.
    //
    {
        name_elt *ne = tname_tab_find("0", 0);
        if (!ne) {
            CDp_cnode *pc = new CDp_cnode;
            pc->set_term_name("0");
            pc->set_next_prp(cn_btprps);
            cn_btprps = pc;

            ne = cn_name_elt_alloc.new_element();
            ne->set(0, CDnetex::name_tab_add("0"), -1, pc);
            add_to_tname_tab(ne);
            add_to_ntab(0, pc);
            ScedErrLog.add_log("new dummy node %s %d", pc->term_name(),
                pc->enode());
        }
    }

    // Set active layer wire nodes without an associated label to -1. 
    // With a label, initialize with a value, consistent with the name
    // table.
    //
    CDsLgen gen(sd);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        CDg gdesc;
        gdesc.init_gen(sd, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (odesc->type() != CDWIRE)
                continue;

            CDp_bnode *pb = (CDp_bnode*)odesc->prpty(P_BNODE);
            if (pb) {
                if (pb->bound()) {
                    // The wire has a bus terminal and label.  Remove any
                    // node property to avoid spurious scalar connectivity.

                    odesc->prptyRemove(P_NODE);

                    // Update the bundle list.  Every BNODE property with
                    // an associated label will have a bundle list, which
                    // is refreshed here for bus wire properties.

                    char *lbl = hyList::string(pb->bound()->label(),
                        HYcvPlain, true);
                    const char *s = lbl;
                    if (s) {
                        while (isspace(*s))
                            s++;
                    }
                    if (!s || !*s) {
                        // Empty label, useless so remove property.
                        odesc->prptyRemove(P_BNODE);
                        delete [] lbl;
                        continue;
                    }
                    CDnetex *nx;
                    if (!CDnetex::parse(s, &nx) || !nx) {
                        // Label parse error, label kept but ignored.
                        ScedErrLog.add_err(
                            "bad wire label text %s, ignored, %s.",
                            s, Errs()->get_error());
                        pb->update_bundle(0);
                        delete [] lbl;
                        continue;
                    }
                    pb->update_bundle(nx);
                    delete [] lbl;

                    CDnetexGen ngen(pb);
                    CDnetName nm;
                    int n;
                    while (ngen.next(&nm, &n)) {

                        // If there is no name, this is a "tap" wire.
                        if (!nm)
                            continue;

                        name_elt *ne = tname_tab_find(Tstring(nm), n);
                        if (!ne) {
                            CDp_cnode *pc = new CDp_cnode;
                            CDnetName tnm = CDnetex::mk_name(Tstring(nm), n);
                            pc->CDp_node::set_term_name(tnm);
                            pc->set_pos(0, ((CDw*)odesc)->points()[0].x,
                                ((CDw*)odesc)->points()[0].y);
                            pc->set_enode(-1);
                            pc->set_next_prp(cn_btprps);
                            cn_btprps = pc;
                            pc->set_flag(TE_BYNAME);

                            ne = cn_name_elt_alloc.new_element();
                            ne->set(cn_count, nm, n, pc);
                            add_to_tname_tab(ne);
                            add_to_ntab(cn_count, pc);
                            new_node();
                            ScedErrLog.add_log("new dummy node %s %d",
                                pc->term_name(), pc->enode());
                        }
                    }
                    continue;
                }
                // No bound label.  The property is useless so
                // remove it.
                odesc->prptyRemove(P_BNODE);
            }

            // Every active wire should have a node property.  A node
            // property is added here if none.  If the wire has a
            // scalar label, the label is bound to the node property. 
            // In this case, we apply a unique starting node number
            // here.  If no label, the node property caches the common
            // node number of all attached terminals.  This simplifies
            // and accelerates determining the node of a wire.  The
            // node value is initialized to -1 in this case.

            CDp_wnode *pn = (CDp_wnode*)odesc->prpty(P_NODE);
            if (!pn) {
                pn = new CDp_wnode;
                pn->set_enode(-1);
                odesc->link_prpty_list(pn);
                continue;
            }

            // Make sure that wire label and name are in sync.
            if (!pn->bound()) {
                pn->set_term_name(0);
                pn->set_enode(-1);
                continue;
            }

            char *lbl = hyList::string(pn->bound()->label(), HYcvPlain, true);
            const char *s = lbl;
            char *tok = lstring::gettok(&s);
            delete [] lbl;
            if (!tok) {
                // label is nothing but white space, remove it.
                delete [] tok;
                odesc->prptyRemove(P_BNODE);
                pn = new CDp_wnode;
                pn->set_enode(-1);
                odesc->link_prpty_list(pn);
                continue;
            }

            int n;
            CDnetName nm;
            if (!CDnetex::parse_bit(tok, &nm, &n, true)) {
                ScedErrLog.add_err("bad assigned net name %s, ignored.\n%s",
                    tok, Errs()->get_error());
                pn->set_term_name(0);
                pn->set_enode(-1);
                delete [] tok;
                continue;
            }
            // The name can be a form like foo<1>.  Both "foo" and
            // "foo<1>" will be in the string table.

            pn->set_term_name(CDnetex::name_tab_add(tok));
            delete [] tok;

            // If no name, then a "tap" wire.  These will be
            // assigned later when we know the context.
            if (!nm) {
                pn->set_enode(-1);
                continue;
            }

            name_elt *ne = tname_tab_find(Tstring(nm), n);
            if (ne)
                add_to_ntab(ne->nodenum(), pn);
            else {
                ne = cn_name_elt_alloc.new_element();
                ne->set(cn_count, nm, n, pn);
                add_to_tname_tab(ne);
                add_to_ntab(cn_count, pn);
                new_node();
            }
        }
    }

    // Add temporary wires across "shorted" NOPHYS devices.
    //
    if (lvsmode)
        init_nophys_shorts();

    // Initialize instances.

    CDg gdesc;
    gdesc.init_gen(sd, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            continue;
        CDelecCellType tp = msdesc->elecCellType();

        if (tp == CDelecGnd) {
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            if (pn)
                pn->set_enode(0);
        }
        else if (tp == CDelecTerm)
            init_terminal(cdesc);
        else if (tp == CDelecDev) {
            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
            if (pr)
                pr->setup(cdesc);
            else {
                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next())
                    pn->set_enode(-1);
            }
        }
        else if (tp == CDelecMacro || tp == CDelecSubc) {
            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
            if (pr)
                pr->setup(cdesc);
            else {
                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next())
                    pn->set_enode(-1);
            }
        }
    }

    SCD()->renumberInstances(sd);

    sd->updateTermNames(CDs::UTNinstances);
}


bool
cScedConnect::init_terminal(CDc *cdesc)
{
    // We know that cdesc is a terminal device, but we don't really
    // know what kind until we look at the label string.  This is
    // where terminal devices are actually configured.

    CDs *msd = cdesc->masterCell();
    if (!msd)
        return (false);

    // Check for valid name property.
    CDp_cname *pna = (CDp_cname*)msd->prpty(P_NAME);
    if (!pna || (pna->key() != P_NAME_TERM &&
            pna->key() != P_NAME_BTERM_DEPREC))
        return (false);
    if (pna->key() == P_NAME_BTERM_DEPREC) {
        // We shouldn't see this anymore, translate to normal terminal
        // prefix.
        char tbf[2];
        tbf[0] = P_NAME_TERM;
        tbf[1] = 0;
        pna->set_name_string(tbf);
    }

    // We accept either a single node or bnode, but not both.
    CDp_snode *ppn = (CDp_snode*)msd->prpty(P_NODE);
    CDp_bsnode *ppb = (CDp_bsnode*)msd->prpty(P_BNODE);
    if ((!ppn && !ppb) || (ppn && ppb))
        return (false);
    if (ppn && ppn->next())
        return (false);
    if (ppb && ppb->next())
        return (false);

    // Check for bound instance label.
    pna = (CDp_cname*)cdesc->prpty(P_NAME);
    if (!pna || !pna->bound())
        return (false);
    if (pna->key() == P_NAME_BTERM_DEPREC) {
        // We shouldn't see this anymore, translate to normal terminal
        // prefix.
        char tbf[2];
        tbf[0] = P_NAME_TERM;
        tbf[1] = 0;
        pna->set_name_string(tbf);
    }
    char *label = hyList::string(pna->bound()->label(), HYcvPlain, true);
    if (!label)
        return (false);

    // Parse the label as a net expression.
    CDnetex *netex;
    if (!CDnetex::parse(label, &netex) || !netex) {
        ScedErrLog.add_err("bad terminal device label name %s, ignored.\n%s",
            label, Errs()->get_error());
        delete [] label;
        return (false);
    }
    delete [] label;

    int x, y;
    if (ppn)
        ppn->get_schem_pos(&x, &y);
    else
        ppb->get_schem_pos(&x, &y);
    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(cdesc);
    stk.TPoint(&x, &y);
    stk.TPop();

    CDnetName nm;
    int beg = -1, end = -1;
    if (netex->is_scalar(&nm) ||
            (netex->is_simple(&nm, &beg, &end) && nm && beg == end)) {
        // A scalar or 1-bit connector.
        CDnetex::destroy(netex);

        cdesc->prptyRemove(P_BNODE);
        CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
        if (!pn) {
            pn = new CDp_cnode;
            cdesc->link_prpty_list(pn);
        }
        pn->set_pos(0, x, y);
        pn->CDp_node::set_term_name(nm);

        name_elt *ne = tname_tab_find(Tstring(nm), beg);
        if (ne)
            add_to_ntab(ne->nodenum(), pn);
        else {
            ne = cn_name_elt_alloc.new_element();
            ne->set(cn_count, nm, beg, pn);
            add_to_tname_tab(ne);
            add_to_ntab(cn_count, pn);
            new_node();
        }
    }
    else {
        // A multi-conductor (bundle or vector) connector.

        cdesc->prptyRemove(P_NODE);
        CDp_bcnode *pn = (CDp_bcnode*)cdesc->prpty(P_BNODE);
        if (!pn) {
            pn = new CDp_bcnode;
            cdesc->link_prpty_list(pn);
        }
        pn->set_pos(0, x, y);
        pn->update_bundle(netex);

        CDnetexGen ngen(pn);
        int n;
        while (ngen.next(&nm, &n)) {
            if (!nm) {
                char *tn = pn->id_text();
                ScedErrLog.add_err(
                    "bad terminal device label name %s, ignored.", tn);
                delete [] tn;
                continue;
            }
            name_elt *ne = tname_tab_find(Tstring(nm), n);
            if (!ne) {
                CDp_cnode *pc = new CDp_cnode;
                CDnetName tnm = CDnetex::mk_name(Tstring(nm), n);
                pn->CDp_bnode::set_term_name(tnm);
                pc->set_pos(0, x, y);
                pc->set_enode(-1);
                pc->set_next_prp(cn_btprps);
                cn_btprps = pc;
                pc->set_flag(TE_BYNAME);

                ne = cn_name_elt_alloc.new_element();
                ne->set(cn_count, nm, n, pc);
                add_to_tname_tab(ne);
                add_to_ntab(cn_count, pc);
                new_node();
                ScedErrLog.add_log("new dummy node %s %d", pc->term_name(),
                    pc->enode());
            }
        }
    }
    return (true);
}


// Effectively short out devices that have the "shorted" NOPHYS property
// set.  This is done for LVS.
//
void
cScedConnect::init_nophys_shorts()
{
    CDg gdesc;
    gdesc.init_gen(cn_sdesc, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            continue;

        CDelecCellType tp = msdesc->elecCellType();
        if (!isDevOrSubc(tp))
            continue;

        CDp *pnp = cdesc->prpty(P_NOPHYS);
        if (!pnp)
            continue;
        const char *ps = pnp->string();
        if (!ps)
            continue;
        if (*ps != 's' && *ps != 'S')
            continue;
        // We've found a NOPHYS property whose string starts with s or
        // S (e.g., "shorted").  This indicates a device that should
        // be shorted in the schematic when comparing with physical
        // data.
        //
        // This is implemented by creating a temporary wire that ties
        // the terminals together.  This should work whether or not
        // the device is vectorized.

        int ncnt = 0;
        CDp_cnode *pnode = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pnode; pnode = pnode->next())
            ncnt++;
        if (ncnt < 2)
            continue; // Hmmm, wtf?

        Wire wire;
        wire.points = new Point[ncnt];
        wire.numpts = ncnt;
        ncnt = 0;
        pnode = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pnode; pnode = pnode->next()) {
            int x, y;
            if (!pnode->get_pos(0, &x, &y))
                continue;
            wire.points[ncnt].set(x, y);
            ncnt++;
        }

        // Find an electrically active layer.
        CDlgen gen(Electrical);
        CDl *ld;
        while ((ld = gen.next()) != 0) {
            if (ld->isWireActive())
                break;
        }
        if (!ld)
            continue;

        CDw *wd = new CDw(ld, &wire);
        cn_tmp_wires = new CDol(wd, cn_tmp_wires);

        CDp_wnode *pnw = new CDp_wnode;
        pnw->set_enode(-1);
        wd->set_prpty_list(pnw);
    }
}


// For each terminal, find and accumulate in the node table the
// connected wires and instance contacts.
//
void
cScedConnect::connect()
{
    // Table to keep track of wires we've processed.
    delete cn_wire_tab;
    cn_wire_tab = new SymTab(false, false);

    // Insert temp wires into the database temporarily.
    for (CDol *ol = cn_tmp_wires; ol; ol = ol->next)
        cn_sdesc->db_insert(ol->odesc);

    ScedErrLog.add_log("connecting named wires");

    // Establish connectivity to named wires.  This includes unnamed
    // or tap wires where a name can be inferred by a connected cell
    // terminal.
    {
        CDsLgen gen(cn_sdesc);
        CDl *ld;
        while ((ld = gen.next()) != 0) {
            if (!ld->isWireActive())
                continue;
            CDg gdesc;
            gdesc.init_gen(cn_sdesc, ld);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (!odesc->is_normal())
                    continue;
                if (odesc->type() != CDWIRE)
                    continue;
                CDw *wd = (CDw*)odesc;
                if (push(wd)) {
                    iterate_wire();
                    pop();
                }
            }
        }
    }

    // Connect the terminals on unnamed wire nets.
    {
        // This is a two-pass operation.  On the first pass, identfy
        // nets that connect to a scalar terminal.  These collapse to
        // scalar nets.  The second pass will deal with remaining
        // vector nets.  This avoids trying to connect two
        // incompatible vector pins before connecting to a scalar and
        // collapsing.  If the net is first identified as scalar, the
        // two scalar-to-vector connections are perfectly ok.

        for (cn_pass = 0; cn_pass <= 1; cn_pass++) {
            ScedErrLog.add_log("connecting unnamed pass %d", cn_pass+1);
            CDsLgen gen(cn_sdesc);
            CDl *ld;
            while ((ld = gen.next()) != 0) {
                if (!ld->isWireActive())
                    continue;
                CDg gdesc;
                gdesc.init_gen(cn_sdesc, ld);
                CDo *odesc;
                while ((odesc = gdesc.next()) != 0) {
                    if (!odesc->is_normal())
                        continue;
                    if (odesc->type() != CDWIRE)
                        continue;
                    CDw *wd = (CDw*)odesc;
                    if (push_unnamed(wd)) {
                        iterate_unnamed_wire();
                        pop();
                    }
                }
            }
        }
        cn_pass = 0;
    }

    // Remove the temp wires from the database.
    for (CDol *ol = cn_tmp_wires; ol; ol = ol->next)
        cn_sdesc->db_remove(ol->odesc);

    // Connect cell terminals to the node of any co-located
    // node-setting dummies.
    {
        for (CDp_cnode *pn = cn_ndprps; pn; pn = pn->next()) {
            Point p;
            if (!pn->get_pos(0, &p.x, &p.y))
                continue;
            BBox BB(p.x, p.y, p.x, p.y);
            CDg gdesc;
            gdesc.init_gen(cn_sdesc, CellLayer(), &BB);
            CDc *cdesc;
            while ((cdesc = (CDc*)gdesc.next()) != 0) {
                CDs *msdesc = cdesc->masterCell();
                if (!msdesc)
                    continue;
                CDelecCellType tp = msdesc->elecCellType();
                if (isDevOrSubc(tp)) {
                    // A device or subcircuit.

                    bool found = false;
                    bool issym = (tp == CDelecDev);
                    if (!issym)
                        issym = msdesc->isSymbolic();

                    CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
                    if (!pr) {
                        CDp_cnode *pnc = (CDp_cnode*)cdesc->prpty(P_NODE);
                        for ( ; pnc; pnc = pnc->next()) {
                            if (pnc->has_flag(TE_BYNAME))
                                continue;
                            if (issym) {
                                if (pnc->has_flag(TE_SYINVIS))
                                    continue;
                            }
                            else {
                                if (pnc->has_flag(TE_SCINVIS))
                                    continue;
                            }

                            int x, y;
                            if (!pnc->get_pos(0, &x, &y))
                                continue;
                            if (x == p.x && y == p.y) {
                                connect_nodes(pn, pnc);
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found)
                        break;
                }
            }
        }
    }

    // All that remain are pin-to-pin connections.

    // Establish connectivity to the device and subcircuit terminals,
    // and terminal devices, established by co-located pins.
    {
        // We want spatial order for consistent node numbering, don't
        // use CDm_gen here.
        CDg gdesc;
        gdesc.init_gen(cn_sdesc, CellLayer());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDs *msdesc = cdesc->masterCell();
            if (!msdesc)
                continue;
            CDelecCellType tp = msdesc->elecCellType();

            if (tp == CDelecGnd || tp == CDelecTerm) {
                // A ground or terminal device.
                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                if (pn) {
                    int x, y;
                    if (pn->get_pos(0, &x, &y))
                        connect_inst_to_inst(cdesc, pn, 0, 0, x, y);
                    continue;
                }
                CDp_bcnode *pb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
                if (pb && pb->has_name()) {
                    int x, y;
                    if (pb->get_pos(0, &x, &y))
                        connect_inst_to_inst(cdesc, 0, pb, 0, x, y);
                    continue;
                }
            }
            else if (isDevOrSubc(tp)) {
                // A device or subcircuit.

                bool issym = (tp == CDelecDev);
                if (!issym)
                    issym = msdesc->isSymbolic();

                CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);

                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    if (pn->has_flag(TE_BYNAME))
                        continue;
                    if (issym) {
                        if (pn->has_flag(TE_SYINVIS))
                            continue;
                    }
                    else {
                        if (pn->has_flag(TE_SCINVIS))
                            continue;
                    }
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!pn->get_pos(ix, &x, &y))
                            break;
                        connect_inst_to_inst(cdesc, pn, 0, pr, x, y);
                    }
                }

                CDp_bcnode *pb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
                for ( ; pb; pb = pb->next()) {
                    if (issym) {
                        if (pb->has_flag(TE_SYINVIS))
                            continue;
                    }
                    else {
                        if (pb->has_flag(TE_SCINVIS))
                            continue;
                    }
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!pb->get_pos(ix, &x, &y))
                            break;
                        connect_inst_to_inst(cdesc, 0, pb, pr, x, y);
                    }
                }
            }
        }
    }

    // Connect scalar cell terminals to co-located pins.
    {
        CDp_snode *ps = (CDp_snode*)cn_sdesc->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            if (ps->has_flag(TE_BYNAME | TE_SCINVIS))
                continue;
            int x, y;
            ps->get_schem_pos(&x, &y);
            connect_cell_to_inst(ps, 0, x, y);
        }
    }

    // Connect vector cell terminals to co-located pins.
    {
        CDp_bsnode *pb = (CDp_bsnode*)cn_sdesc->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            if (pb->has_flag(TE_SCINVIS))
                continue;
            int x, y;
            pb->get_schem_pos(&x, &y);
            connect_cell_to_inst(0, pb, x, y);
        }
    }

    // Adding a scalar cell terminal can cause a split net.  Here we
    // detect this and merge the nets.
    //
    for (CDp_snode *pn = (CDp_snode*)cn_sdesc->prpty(P_NODE); pn;
            pn = pn->next()) {
        if (!pn->has_flag(TE_BYNAME | TE_SCINVIS)) {
            // An ordinary terminal, set the node.

            int x, y;
            pn->get_schem_pos(&x, &y);
            int npos = find_node(x, y);
            if (npos < 0)
                continue;

            int nnam;
            find_node(Tstring(pn->term_name()), &nnam);
            if (nnam < 0)
                continue;

            if (npos != nnam) {
                // Interesting case.  Presently, the terminal connects
                // to one net by position, but the name matches that
                // of another net.  This is treated as one (split)
                // net, the two are merged here.

                merge(nnam, npos);
            }
        }
    }

    // Make sure that all scalar instance terminal nodes have an
    // assigned value.  An instance terminal that connects to nothing
    // thus far has no node assignment.
    {
        CDg gdesc;
        gdesc.init_gen(cn_sdesc, CellLayer());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDs *msdesc = cdesc->masterCell();
            if (!msdesc)
                continue;

            CDelecCellType tp = msdesc->elecCellType();
            if (isDevOrSubc(tp)) {
                // A device or subcircuit.

                bool issym = (tp == CDelecDev);
                if (!issym)
                    issym = msdesc->isSymbolic();

                CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
                if (pr) {
                    CDgenRange rgen(pr);
                    int cnt = 0;
                    while (rgen.next(0)) {
                        CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                        for ( ; pn; pn = pn->next()) {
/*XXX
                            if (pn->has_flag(TE_BYNAME))
                                continue;
                            if (issym) {
                                if (pn->has_flag(TE_SYINVIS))
                                    continue;
                            }
                            else {
                                if (pn->has_flag(TE_SCINVIS))
                                    continue;
                            }
*/
                            CDp_cnode *pn1;
                            if (cnt == 0)
                                pn1 = pn;
                            else
                                pn1 = pr->node(0, cnt, pn->index());
                            if (pn1->enode() < 0) {
                                add_to_ntab(cn_count, pn1);
                                new_node();
                            }
                        }
                        cnt++;
                    }
                }
                else {
                    CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                    for ( ; pn; pn = pn->next()) {
//XXX Need to assign a node if these are referenced by an open bus terminal.
/*XXX
                        if (pn->has_flag(TE_BYNAME))
                            continue;
                        if (issym) {
                            if (pn->has_flag(TE_SYINVIS))
                                continue;
                        }
                        else {
                            if (pn->has_flag(TE_SCINVIS))
                                continue;
                        }
*/
                        if (pn->enode() < 0) {
                            add_to_ntab(cn_count, pn);
                            new_node();
                        }
                    }
                }
            }
        }
    }

    // Compact the node numbering.
    reduce(1);

    if (ScedErrLog.log_connect()) {
        SymTab *sytmp = new SymTab(false, false);
        for (int i = 0; i < cn_count; i++) {
            for (node_list *n = cn_ntab[i]; n; n = n->next()) {
                if (n->node()->enode() != i)
                    ScedErrLog.add_err(
                    "final node table numbering error, wrong node number.");
                if (SymTab::get(sytmp, (unsigned long)n->node()) != ST_NIL)
                    ScedErrLog.add_err(
                    "final node table numbering error, duplicate property.");
                else
                   sytmp->add((unsigned long)n->node(), 0, false);
            }
        }
        delete sytmp;
    }

    delete cn_wire_tab;
    cn_wire_tab = 0;
}


bool
cScedConnect::push(const CDw *wdesc)
{
    if (SymTab::get(cn_wire_tab, (unsigned long)wdesc) != ST_NIL)
        // Already processed this wire.
        return (false);

    CDnetex *netex;
    if (cn_wire_stack) {
        if (!is_connected(wdesc))
            return (false);
        // If the wire connects at a higher level, defer.
        if (is_connected_higher(wdesc))
            return (false);
        if (!is_compatible(wdesc, &netex))
            return (false);
        if (ScedErrLog.log_connect()) {
            sLstr lstr;
            CDnetex::print_all(netex, &lstr);
            ScedErrLog.add_log("pushed %s", lstr.string());
        }
    }
    else {
        netex = get_netex(wdesc);
        if (netex && !netex->first_name()) {
            // A tap wire, no good.
            CDnetex::destroy(netex);
            return (false);
        }
        if (!netex) {
            // Unnamed wire, see if we can infer a name from a
            // connected terminal.

            if (!infer_name(wdesc, &netex))
                return (false);
        }
        if (ScedErrLog.log_connect()) {
            sLstr lstr;
            CDnetex::print_all(netex, &lstr);
            ScedErrLog.add_log("top pushed %s", lstr.string());
        }
    }
    cn_wire_stack = new cstk_elt(netex, wdesc, cn_wire_stack);
    cn_wire_tab->add((unsigned long)wdesc, 0, false);
    return (true);
}


bool
cScedConnect::push_unnamed(const CDw *wdesc)
{
    if (SymTab::get(cn_wire_tab, (unsigned long)wdesc) != ST_NIL)
        // Already processed this wire.
        return (false);

    // We know that this wire is unnamed, we have already processed all
    // named wires.

    if (cn_wire_stack) {
        if (!is_connected(wdesc))
            return (false);
        if (is_connected_higher(wdesc))
            return (false);
    }
    else {
        if (!find_a_terminal(wdesc))
            return (false);
    }

    cn_wire_stack = new cstk_elt(0, wdesc, cn_wire_stack);
    cn_wire_tab->add((unsigned long)wdesc, 0, false);
    return (true);
}


void
cScedConnect::pop()
{
    ScedErrLog.add_log("popped");
    cstk_elt *e = cn_wire_stack;
    if (cn_wire_stack)
        cn_wire_stack = cn_wire_stack->next();
    delete e;
}


// The passed wire has no name.  Look for a terminal that touches the
// wire, that will provide a name for the wire.  The new netex is
// returned.
//
// If the wire is scalar and the return is successful, it will have a
// non-negative node number.
//
bool
cScedConnect::infer_name(const CDw *wdesc, CDnetex **pnx)
{
    if (!wdesc)
        return (false);

    if (pnx)
        *pnx = 0;

    // Look for scalar connections first.  If none, we'll check the
    // vector connections.

    // A wire node property is required for a scalar connection. 
    // We'll be permissibe here and allow a wire without a node
    // property to make bus connections.

    CDp_node *pnw = (CDp_node*)wdesc->prpty(P_NODE);
    if (pnw) {

        // Check named scalar cell terminals.
        {
            CDp_snode *ps = (CDp_snode*)cn_sdesc->prpty(P_NODE);
            for ( ; ps; ps = ps->next()) {
                if (ps->has_flag(TE_BYNAME | TE_SCINVIS))
                    continue;
                if (!ps->get_term_name())
                    continue;
                Point p;
                ps->get_schem_pos(&p.x, &p.y);
                if (wdesc->has_vertex_at(p)) {
                    if (pnx) {
                        const char *nmstr = Tstring(ps->get_term_name());
                        if (!CDnetex::parse(nmstr, pnx)) {
                            ScedErrLog.add_err("bad terminal name %s, %s.",
                                ps->term_name(), Errs()->get_error());
                            continue;
                        }
                        if (!*pnx)
                            continue;
                    }
                    connect_nodes(pnw, ps);
                    return (true);
                }
            }
        }

        // Check the name-mapping properties.
        {
            CDp_cnode *pn = cn_ndprps;
            for ( ; pn; pn = pn->next()) {
                Point p;
                if (pn->get_pos(0, &p.x, &p.y) && wdesc->has_vertex_at(p)) {
                    if (pnx) {
                        const char *nmstr = pn->string();
                        if (!CDnetex::parse(nmstr, pnx)) {
                            ScedErrLog.add_err("bad assigned name %s, %s.",
                                nmstr, Errs()->get_error());
                            continue;
                        }
                        if (!*pnx)
                            continue;
                    }
                    connect_nodes(pnw, pn);
                    return (true);
                }
            }
        }

        // Check scalar terminal library devices.
        {
            CDg gdesc;
            gdesc.init_gen(cn_sdesc, CellLayer(), &wdesc->oBB());
            CDc *cdesc;
            while ((cdesc = (CDc*)gdesc.next()) != 0) {
                CDs *msdesc = cdesc->masterCell();
                if (!msdesc)
                    continue;

                CDelecCellType tp = msdesc->elecCellType();
                if (tp != CDelecGnd && tp != CDelecTerm)
                    continue;

                // A ground or scalar terminal device.
                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                if (!pn)
                    continue;
                Point p;
                if (pn->get_pos(0, &p.x, &p.y) && wdesc->has_vertex_at(p)) {

                    char *label = 0;
                    CDp_cname *pna = (CDp_cname*)cdesc->prpty(P_NAME);
                    if (!pna) {
                        // Must be a ground terminal.
                        if (tp != CDelecGnd)
                            continue;
                        label = lstring::copy("0");
                    }
                    else {
                        if (!pna->bound())
                            continue;
                        label = hyList::string(pna->bound()->label(),
                            HYcvPlain, true);
                    }
                    if (!label)
                        continue;
                    if (!CDnetex::parse(label, pnx)) {
                        ScedErrLog.add_err("bad terminal label name %s, %s.",
                            label, Errs()->get_error());
                        delete [] label;
                        continue;
                    }
                    if (!*pnx) {
                        delete [] label;
                        continue;
                    }
                    connect_nodes(pnw, pn);
                    delete [] label;
                    return (true);
                }
            }
        }
    }

    // No scalar candidates found, we'll try vectors.

    // Check named vector cell terminals.
    {
        CDp_bsnode *pb = (CDp_bsnode*)cn_sdesc->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            if (!pb->has_name())
                continue;
            if (pb->has_flag(TE_SCINVIS))
                continue;
            Point p;
            pb->get_schem_pos(&p.x, &p.y);
            if (wdesc->has_vertex_at(p)) {
                // Found a named cell bus terminal on the wire, use its
                // bundle spec.

                if (pnx)
                    *pnx = pb->get_netex();
                if (ScedErrLog.log_connect()) {
                    sLstr lstr;
                    CDnetex::print_all((*pnx), &lstr);
                    ScedErrLog.add_log("found terminal %s", lstr.string());
                }
                return (true);
            }
        }
    }

    // Check vector terminal library devices.
    {
        CDg gdesc;
        gdesc.init_gen(cn_sdesc, CellLayer(), &wdesc->oBB());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDs *msdesc = cdesc->masterCell();
            if (!msdesc)
                continue;

            CDelecCellType tp = msdesc->elecCellType();
            if (tp == CDelecTerm) {
                // A terminal device.
                CDp_bcnode *pcn = (CDp_bcnode*)cdesc->prpty(P_BNODE);
                if (!pcn || !pcn->has_name())
                    continue;
                Point p;
                if (pcn->get_pos(0, &p.x, &p.y) && wdesc->has_vertex_at(p)) {
                    if (pnx)
                        *pnx = pcn->get_netex();
                    if (ScedErrLog.log_connect()) {
                        sLstr lstr;
                        CDnetex::print_all((*pnx), &lstr);
                        ScedErrLog.add_log("found terminal device %s",
                            lstr.string());
                    }
                    return (true);
                }
            }
        }
    }

    // No luck.
    return (false);
}


// The wire is unnamed.  Look for an unnamed terminal that connects to
// the wire, return true if one is found.
//
// On success for a scalar terminal, make sure that the wire has an
// assigned node.  For a vector terminal, save it for connecting
// when iterating over terminals.
//
bool
cScedConnect::find_a_terminal(const CDw *wdesc)
{
    // Clear the reference terminal store.  This is only used for
    // vector terminals, or vectored instance scalar terminals.
    cn_un_bnode = 0;
    cn_un_node = 0;
    cn_un_cdesc = 0;

    if (!wdesc)
        return (false);

    if (cn_pass == 0) {
        // On the first pass, we return scalar connections only.

        CDp_node *pnw = (CDp_node*)wdesc->prpty(P_NODE);
        if (!pnw)
            return (false);

        // Check unnamed scalar cell terminals.
        {
            CDp_snode *ps = (CDp_snode*)cn_sdesc->prpty(P_NODE);
            for ( ; ps; ps = ps->next()) {
                if (ps->has_flag(TE_BYNAME))
                    continue;
                if (ps->get_term_name())
                    continue;
                Point p;
                ps->get_schem_pos(&p.x, &p.y);
                if (wdesc->has_vertex_at(p)) {
                    connect_nodes(pnw, ps);
                    return (true);
                }
            }
        }

        // Check scalar instance terminals.
        CDg gdesc;
        gdesc.init_gen(cn_sdesc, CellLayer(), &wdesc->oBB());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDs *msdesc = cdesc->masterCell();
            if (!msdesc)
                continue;

            CDelecCellType tp = msdesc->elecCellType();
            if (isDevOrSubc(tp)) {
                // A device or subcircuit.
                bool issym = (tp == CDelecDev);
                if (!issym)
                    issym = msdesc->isSymbolic();

                CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
                if (pr)
                    continue;

                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    for (int i = 0; i < wdesc->numpts(); i++) {
                        if (in_contact(pn, issym, wdesc->points()[i].x,
                                wdesc->points()[i].y)) {
                            connect_nodes(pnw, pn);
                            return (true);
                        }
                    }
                }
            }
        }
        // No scalar terminals found.
        return (false);
    }

    if (cn_pass == 1) {
        // On the second pass, check vector terminals only.  The
        // scalars were discovered in the first pass.

        // Check unnamed vector cell terminals.
        {
            CDp_bsnode *pb = (CDp_bsnode*)wdesc->prpty(P_BNODE);
            for ( ; pb; pb = pb->next()) {
                if (pb->has_flag(TE_SCINVIS))
                    continue;
                if (pb->has_name())
                    continue;
                Point p;
                pb->get_schem_pos(&p.x, &p.y);
                if (wdesc->has_vertex_at(p)) {
                    // Save the property, null cn_un_cdesc indicates a
                    // cell property.
                    ScedErrLog.add_log("in find_a_terminal, found %s",
                        pb->term_name());
                    cn_un_bnode = pb;
                    return (true);
                }
            }
        }

        // Check vector instance terminals.
        CDg gdesc;
        gdesc.init_gen(cn_sdesc, CellLayer(), &wdesc->oBB());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDs *msdesc = cdesc->masterCell();
            if (!msdesc)
                continue;

            CDelecCellType tp = msdesc->elecCellType();
            if (isDevOrSubc(tp)) {
                // A device or subcircuit.
                bool issym = (tp == CDelecDev);
                if (!issym)
                    issym = msdesc->isSymbolic();

                CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
                if (pr) {
                    CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                    for ( ; pn; pn = pn->next()) {
                        for (int i = 0; i < wdesc->numpts(); i++) {
                            if (in_contact(pn, issym, wdesc->points()[i].x,
                                    wdesc->points()[i].y)) {
                                // Save the property, and cdesc.
                                ScedErrLog.add_log(
                                    "in find_a_terminal, found %s",
                                    pn->term_name());
                                cn_un_node = pn;
                                cn_un_cdesc = cdesc;
                                return (true);
                            }
                        }
                    }
                }

                CDp_bcnode *pb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
                for ( ; pb; pb = pb->next()) {
                    for (int i = 0; i < wdesc->numpts(); i++) {
                        if (in_contact(pb, issym, wdesc->points()[i].x,
                                wdesc->points()[i].y)) {
                            // Save the property, and cdesc.

                            cn_un_bnode = pb;
                            cn_un_cdesc = cdesc;
                            ScedErrLog.add_log(
                                "in find_a_terminal, found %s",
                                pb->term_name());
                            return (true);
                        }
                    }
                }
            }
        }
    }

    // No luck.
    return (false);
}


// Return true if wdesc and top-of-stack share a vertex location.
//
bool
cScedConnect::is_connected(const CDw *wdesc)
{
    if (!cn_wire_stack || !wdesc)
        return (false);
    const CDw *wd = cn_wire_stack->wdesc();
    for (int i = 0; i < wd->numpts(); i++) {
        if (wdesc->has_vertex_at(wd->points()[i]))
            return (true);
    }
    return (false);
}


// Return true if the wire shares a vertex with a wire in the stack
// other than top-of-stack.
//
bool
cScedConnect::is_connected_higher(const CDw *wdesc)
{
    if (!cn_wire_stack || !wdesc)
        return (false);
    for (cstk_elt *cx = cn_wire_stack->next(); cx; cx = cx->next()) {
        const CDw *wd = cx->wdesc();
        for (int i = 0; i < wd->numpts(); i++) {
            if (wdesc->has_vertex_at(wd->points()[i]))
                return (true);
        }
    }
    return (false);
}


// Return a netex for the wire, if one can be obtained.  This should
// succeed for any named wire.
//
CDnetex *
cScedConnect::get_netex(const CDw *wdesc)
{
    if (!wdesc)
        return (0);

    CDp_bnode *pb = (CDp_bnode*)wdesc->prpty(P_BNODE);
    if (pb)
        return (pb->get_netex());

    CDp_node *pn = (CDp_node*)wdesc->prpty(P_NODE);
    if (pn && pn->get_term_name()) {
        CDnetex *nx;
        if (!CDnetex::parse(Tstring(pn->get_term_name()), &nx)) {
            ScedErrLog.add_err("bad name for scalar label %s, %s.",
                Tstring(pn->get_term_name()), Errs()->get_error());
            return (0);
        }
        return (nx);
    }
    return (0);
}


// Return true if the wire can connect to the top-of-stack.  If so,
// return a CDnetex for the passed wire.  This will override any
// bundle spec in the passed wire properties, and provides the names
// to "tap" wires and unlabeled wires.  This must be freed by the
// caller when done.
//
bool
cScedConnect::is_compatible(const CDw *wdesc, CDnetex **pnetex)
{
    if (pnetex)
        *pnetex = 0;
    if (!cn_wire_stack || !wdesc)
        return (false);

    CDnetex *nx = get_netex(wdesc);
    if (nx) {
        if (!CDnetex::check_set_compatible(nx, cn_wire_stack->netex())) {
            if (!nx->first_name()) {
                // A tap wire, don't complain.  A tap wire may connect
                // to other tap wires, along with the wire being
                // tapped.  Just ignore that the taps are
                // "incompatible" mutually.

                CDnetex::destroy(nx);
                return (false);
            }

            char *tn1 = CDnetex::id_text(cn_wire_stack->netex());
            char *tn2 = CDnetex::id_text(nx);
            ScedErrLog.add_err(
                "warning: incompatible expressions in connected named "
                "subnets.\nExpressions are %s and %s.", tn1, tn2);
            delete [] tn1;
            delete [] tn2;
            CDnetex::destroy(nx);
            return (false);
        }
        if (pnetex)
            *pnetex = nx;
        else
            CDnetex::destroy(nx);
        return (true);
    }
    if (wdesc->prpty(P_NODE)) {
        // An unlabeled wire with a node property will act like an
        // extension of the connected wire.

        if (pnetex)
            *pnetex = CDnetex::dup(cn_wire_stack->netex());
        return (true);
    }

    // Wire with no relevant properties, just ignore.
    return (false);
}


void
cScedConnect::iterate_wire()
{
    if (!cn_wire_stack)
        return;

    connect_wires();

    // Recursively traverse the wire tree.
    //
    const CDw *wdesc = cn_wire_stack->wdesc();
    int num = wdesc->numpts();
    const Point *pts = wdesc->points();
    for (int i = 0;  i < num; i++) {
        BBox BB(pts[i].x, pts[i].y, pts[i].x, pts[i].y);
        CDg gdesc;
        CDsLgen gen(cn_sdesc);
        CDl *ld;
        while ((ld = gen.next()) != 0) {
            if (!ld->isWireActive())
                continue;
            gdesc.init_gen(cn_sdesc, ld, &BB);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->type() != CDWIRE)
                    continue;
                CDw *wd = (CDw*)odesc;
                if (push(wd)) {
                    iterate_wire();
                    pop();
                }
            }
        }
    }
    connect_to_terminals();
}


void
cScedConnect::iterate_unnamed_wire()
{
    if (!cn_wire_stack)
        return;

    connect_wires();

    // Recursively traverse the wire tree,
    //
    const CDw *wdesc = cn_wire_stack->wdesc();
    int num = wdesc->numpts();
    const Point *pts = wdesc->points();
    for (int i = 0;  i < num; i++) {
        BBox BB(pts[i].x, pts[i].y, pts[i].x, pts[i].y);
        CDg gdesc;
        CDsLgen gen(cn_sdesc);
        CDl *ld;
        while ((ld = gen.next()) != 0) {
            if (!ld->isWireActive())
                continue;
            gdesc.init_gen(cn_sdesc, ld, &BB);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->type() != CDWIRE)
                    continue;
                CDw *wd = (CDw*)odesc;
                if (push_unnamed(wd)) {
                    iterate_unnamed_wire();
                    pop();
                }
            }
        }
    }
    connect_to_terminals();
}


// Propagate the node number in 1-bit wires.  Warning, this is called
// for unnamed wires where the netex entries are null.
//
void
cScedConnect::connect_wires()
{
    if (!cn_wire_stack || !cn_wire_stack->next())
        return;
    if (cn_wire_stack->next()->netex() &&
            !cn_wire_stack->next()->netex()->is_unit())
        return;
    const CDw *wdesc = cn_wire_stack->next()->wdesc();
    // The wdesc wire is a 1-bit wire.

    CDp_node *pn0 = (CDp_node*)wdesc->prpty(P_NODE);
    if (!pn0 || pn0->enode() < 0)
        return;
    const CDw *wd = cn_wire_stack->wdesc();
    CDp_node *pn = (CDp_node*)wd->prpty(P_NODE);
    if (!pn)
        return;
    connect_nodes(pn0, pn);
}


// Connect touching terminals to the top-of-stack wire.
//
void
cScedConnect::connect_to_terminals()
{
    if (!cn_wire_stack)
        return;
    const CDw *wdesc = cn_wire_stack->wdesc();
    sLstr lstr;
    if (ScedErrLog.log_connect()) {
        CDnetex::print_all(cn_wire_stack->netex(), &lstr);
        if (!lstr.string())
            lstr.add("(unnamed)");
        ScedErrLog.add_log("looking for terminals: %s", lstr.string());
    }

    int num = wdesc->numpts();
    const Point *pts = wdesc->points();
    for (int i = 0;  i < num; i++) {
        int xloc = pts[i].x;
        int yloc = pts[i].y;
        BBox BB(xloc, yloc, xloc, yloc);
        CDg gdesc;
        gdesc.init_gen(cn_sdesc, CellLayer(), &BB);
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {

            CDs *msdesc = cdesc->masterCell();
            if (!msdesc)
                continue;

            CDelecCellType tp = msdesc->elecCellType();
            if (tp == CDelecGnd || tp == CDelecTerm) {
                // A ground or terminal device.
                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                if (pn) {
                    int x, y;
                    if (pn->get_pos(0, &x, &y) && x == xloc && y == yloc) {
                        ScedErrLog.add_log("wire_to_dev %s %s", lstr.string(),
                            pn->term_name());
                        wire_to_term_dev(cdesc, pn, 0);
                    }
                    continue;
                }
                CDp_bcnode *pb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
                if (pb && pb->has_name()) {
                    int x, y;
                    if (pb->get_pos(0, &x, &y) && x == xloc && y == yloc) {
                        ScedErrLog.add_log("wire_to_bdev %s %s", lstr.string(),
                            pb->term_name());
                        wire_to_term_dev(cdesc, 0, pb);
                    }
                    continue;
                }
            }
            else if (isDevOrSubc(tp)) {
                // A device or subcircuit.
                bool issym = (tp == CDelecDev);
                if (!issym)
                    issym = msdesc->isSymbolic();

                CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    if (in_contact(pn, issym, xloc, yloc)) {
                        ScedErrLog.add_log("wire_to_inst %s %s",
                            lstr.string(), pn->term_name());
                        wire_to_inst(cdesc, pn, 0, pr);
                    }
                }

                CDp_bcnode *pb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
                for ( ; pb; pb = pb->next()) {

                    if (issym) {
                        if (pb->has_flag(TE_SYINVIS))
                            continue;
                    }
                    else {
                        if (pb->has_flag(TE_SCINVIS))
                            continue;
                    }
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!pb->get_pos(ix, &x, &y))
                            break;
                    }

                    if (in_contact(pb, issym, xloc, yloc)) {
                        ScedErrLog.add_log("wire_to_binst %s %s",
                            lstr.string(), pb->term_name());
                        wire_to_inst(cdesc, 0, pb, pr);
                    }
                }
            }
        }

        CDp_snode *ps = (CDp_snode*)cn_sdesc->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            if (ps->has_flag(TE_BYNAME | TE_SCINVIS))
                continue;
            int x, y;
            ps->get_schem_pos(&x, &y);
            if (x == xloc && y == yloc) {
                ScedErrLog.add_log("wire_to_cell %s %s", lstr.string(),
                    ps->term_name());
                wire_to_cell(ps, 0);
            }
        }

        CDp_bsnode *pb = (CDp_bsnode*)cn_sdesc->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            if (pb->has_flag(TE_SCINVIS))
                continue;
            int x, y;
            pb->get_schem_pos(&x, &y);
            if (x == xloc && y == yloc) {
                // A scalar wire connects to a bus terminal.  The wire
                // should be a tap, if incompatible this an error.

                CDnetex *nxt = pb->get_netex();
                const CDnetex *nxw = cn_wire_stack->netex();
                if (!CDnetex::check_compatible(nxt, nxw)) {
                    // error
                    ScedErrLog.add_err(
                        "cell terminal contacts incompatible scalar wire "
                        "at %.4f,%.4f.", MICRONS(x), MICRONS(y));
                }
                CDnetex::destroy(nxt);

                /***** Don't flatten cell terminals.
                ScedErrLog.add_log("wire_to_bcell %s %s", lstr.string(),
                    pb->term_name());
                wire_to_cell(0, pb);
                *****/
            }
        }
    }
}


void
cScedConnect::wire_to_term_dev(const CDc *cdesc, CDp_cnode *pcn,
    const CDp_bcnode *pcb)
{
    if (!cn_wire_stack || !cdesc)
        return;
    const CDw *wdesc = cn_wire_stack->wdesc();
    const CDnetex *netex = cn_wire_stack->netex();

    ScedErrLog.add_log("in wire_to_term_dev");
    if (!netex) {
        // Shouldn't get here, since the terminal device will give
        // a name to the wire.
        if (pcn) {
            CDp_node *pn = (CDp_node*)wdesc->prpty(P_NODE);
            if (!pn)
                return;
            connect_nodes(pn, pcn);
        }
    }
    else {
        if (pcn)
            bit_to_named(pcn, 0, netex);
        else if (pcb)
            named_to_named(netex, NetexWrap(pcb).netex());
    }
}


void
cScedConnect::wire_to_inst(const CDc *cdesc, CDp_cnode *pcn,
    const CDp_bcnode *pcb, const CDp_range *pcr)
{
    if (!cn_wire_stack || !cdesc || (!pcn && !pcb))
        return;
    const CDw *wdesc = cn_wire_stack->wdesc();
    const CDnetex *netex = cn_wire_stack->netex();

    ScedErrLog.add_log("in wire_to_inst");
    if (!netex) {
        // Connect to the reference terminal, or to scalar wire.

        if (pcn) {
            if (!pcr) {
                // Connect wire node.
                CDp_node *pn = (CDp_node*)wdesc->prpty(P_NODE);
                if (pn)
                    wbit_to_bit(pn, pcn, 0);
            }
            if (pcn == cn_un_node)
                return;
            if (cn_un_node) {
                if (!cn_un_cdesc)
                    return;
                CDp_range *pr = (CDp_range*)cn_un_cdesc->prpty(P_RANGE);
                // pr should exist
                bit_to_bit(cn_un_node, pr, pcn, pcr);
            }
            else if (cn_un_bnode) {
                if (cn_un_cdesc) {
                    CDp_range *pr = (CDp_range*)cn_un_cdesc->prpty(P_RANGE);
                    bit_to_inst(pcn, pcr, cn_un_cdesc, cn_un_bnode, pr);
                }
                else
                    bit_to_cell(pcn, pcr, (CDp_bsnode*)cn_un_bnode);
            }
            else {
                CDp_node *pn = (CDp_node*)wdesc->prpty(P_NODE);
                if (pn)
                    wbit_to_bit(pn, pcn, pcr);
            }
        }
        else if (pcb) {
            if (pcb == cn_un_bnode)
                return;
            if (cn_un_node) {
                if (!cn_un_cdesc)
                    return;
                CDp_range *pr = (CDp_range*)cn_un_cdesc->prpty(P_RANGE);
                // pr should exist
                bit_to_inst(cn_un_node, pr, cdesc, pcb, pcr);
            }
            else if (cn_un_bnode) {
                if (cn_un_cdesc) {
                    CDp_range *pr = (CDp_range*)cn_un_cdesc->prpty(P_RANGE);
                    inst_to_inst(cdesc, pcb, pcr, cn_un_cdesc, cn_un_bnode, pr);
                }
                else
                    inst_to_cell(cdesc, pcb, pcr, (CDp_bsnode*)cn_un_bnode);
            }
            else {
                CDp_node *pn = (CDp_node*)wdesc->prpty(P_NODE);
                if (pn)
                    wbit_to_inst(pn, cdesc, pcb, pcr);
            }
        }
    }
    else {
        if (pcn)
            bit_to_named(pcn, pcr, netex);
        else if (pcb)
            inst_to_named(cdesc, pcb, pcr, netex);
    }
}


void
cScedConnect::wire_to_cell(CDp_snode *ps, const CDp_bsnode *pb)
{
    if (!cn_wire_stack)
        return;
    const CDw *wdesc = cn_wire_stack->wdesc();
    const CDnetex *netex = cn_wire_stack->netex();

    ScedErrLog.add_log("in wire_to_cell");
    if (!netex) {
        if (ps) {
            // Connect wire node.
            CDp_node *pn = (CDp_node*)wdesc->prpty(P_NODE);
            if (pn)
                wbit_to_bit(pn, ps, 0);

            // The cn_un_node is never set to scalar cell terminals,
            // only vectored instance terminals.

            if (cn_un_node) {
                if (!cn_un_cdesc)
                    return;
                CDp_range *pr = (CDp_range*)cn_un_cdesc->prpty(P_RANGE);
                // pr should exist
                wbit_to_bit(ps, cn_un_node, pr);
            }
            else if (cn_un_bnode) {
                if (cn_un_cdesc) {
                    CDp_range *pr = (CDp_range*)cn_un_cdesc->prpty(P_RANGE);
                    wbit_to_inst(ps, cn_un_cdesc, cn_un_bnode, pr);
                }
                else
                    wbit_to_cell(ps, (CDp_bsnode*)cn_un_bnode);
            }
        }
        else if (pb) {
            if (pb == cn_un_bnode)
                return;
            if (cn_un_node) {
                if (!cn_un_cdesc)
                    return;
                CDp_range *pr = (CDp_range*)cn_un_cdesc->prpty(P_RANGE);
                // pr should exist
                bit_to_cell(cn_un_node, pr, pb);
            }
            else if (cn_un_bnode) {
                if (cn_un_cdesc) {
                    CDp_range *pr = (CDp_range*)cn_un_cdesc->prpty(P_RANGE);
                    inst_to_cell(cn_un_cdesc, cn_un_bnode, pr, pb);
                }
                else
                    cell_to_cell(pb, (CDp_bsnode*)cn_un_bnode);
            }
        }
    }
    else {
        if (ps)
            bit_to_named(ps, 0, netex);
        else if (pb)
            named_to_cell(netex, pb);
    }
}


// Connect instance terminals to co-located terminals of other
// instances.
//
void
cScedConnect::connect_inst_to_inst(const CDc *cdesc_ref, CDp_cnode *pn,
    const CDp_bcnode *pb, const CDp_range *pr, int xloc, int yloc)
{
    CDs *msdesc_ref = cdesc_ref->masterCell();
    if (!msdesc_ref)
        return;
    CDelecCellType tp_ref = msdesc_ref->elecCellType();

    CDg gdesc;
    BBox BB(xloc, yloc, xloc, yloc);
    gdesc.init_gen(cn_sdesc, CellLayer(), &BB);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (cdesc == cdesc_ref)
            continue;
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            continue;

        CDelecCellType tp = msdesc->elecCellType();
        if (tp == CDelecGnd || tp == CDelecTerm) {
            // A ground or terminal device.

            CDp_cnode *pcn = (CDp_cnode*)cdesc->prpty(P_NODE);
            if (pcn) {
                int x, y;
                if (pcn->get_pos(0, &x, &y) && x == xloc && y == yloc) {
                    if (pn)
                        bit_to_bit(pcn, 0, pn, pr);
                    else if (pb) {
                        if (tp_ref == CDelecTerm)
                            bit_to_named(pcn, 0, NetexWrap(pb).netex());
                        else if (isDevOrSubc(tp_ref))
                            bit_to_inst(pcn, 0, cdesc_ref, pb, pr);
                    }
                }
            }
            CDp_bcnode *pcb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
            if (pcb && pcb->has_name()) {
                int x, y;
                if (pcb->get_pos(0, &x, &y) && x == xloc && y == yloc) {
                    if (pn)
                        bit_to_named(pn, pr, NetexWrap(pcb).netex());
                    else if (pb) {
                        if (tp_ref == CDelecTerm)
                            named_to_named(NetexWrap(pcb).netex(),
                                NetexWrap(pb).netex());
                        else if (isDevOrSubc(tp_ref))
                            inst_to_named(cdesc_ref, pb, pr,
                                NetexWrap(pcb).netex());
                    }
                }
            }
        }
        else if (isDevOrSubc(tp)) {
            // A device or subcircuit.

            bool issym = (tp == CDelecDev);
            if (!issym)
                issym = msdesc->isSymbolic();

            CDp_cnode *pcn = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pcn; pcn = pcn->next()) {
                if (in_contact(pcn, issym, xloc, yloc)) {
                    CDp_range *pcr = (CDp_range*)cdesc->prpty(P_RANGE);
                    if (pn)
                        bit_to_bit(pcn, pcr, pn, pr);
                    else if (pb) {
                        if (tp_ref == CDelecTerm)
                            bit_to_named(pcn, pcr, NetexWrap(pb).netex());
                        else if (isDevOrSubc(tp_ref))
                            bit_to_inst(pcn, pcr, cdesc_ref, pb, pr);
                    }
                }
            }

            CDp_bcnode *pcb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
            for ( ; pcb; pcb = pcb->next()) {
                if (in_contact(pcb, issym, xloc, yloc)) {
                    CDp_range *pcr = (CDp_range*)cdesc->prpty(P_RANGE);
                    if (pn)
                        bit_to_inst(pn, pr, cdesc, pcb, pcr);
                    else if (pb) {
                        if (tp_ref == CDelecTerm)
                            inst_to_named(cdesc, pcb, pcr,
                                NetexWrap(pb).netex());
                        else if (isDevOrSubc(tp_ref))
                            inst_to_inst(cdesc, pcb, pcr, cdesc_ref, pb, pr);
                    }
                }
            }
        }
    }
}


// Connect cell terminals to co-located instance terminals.
//
void
cScedConnect::connect_cell_to_inst(CDp_snode *pn, const CDp_bsnode *pb,
    int xloc, int yloc)
{
    if (pn && pn->has_flag(TE_BYNAME))
        return;
    CDg gdesc;
    BBox BB(xloc, yloc, xloc, yloc);
    gdesc.init_gen(cn_sdesc, CellLayer(), &BB);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            continue;

        CDelecCellType tp = msdesc->elecCellType();
        if (tp == CDelecGnd || tp == CDelecTerm) {
            // A ground or terminal device.

            CDp_cnode *pcn = (CDp_cnode*)cdesc->prpty(P_NODE);
            if (pcn) {
                int x, y;
                if (pcn->get_pos(0, &x, &y) && x == xloc && y == yloc) {
                    if (pn)
                        bit_to_bit(pcn, 0, pn, 0);
                    else if (pb)
                        bit_to_cell(pcn, 0, pb);
                }
                continue;
            }
            CDp_bcnode *pcb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
            if (pcb && pcb->has_name()) {
                int x, y;
                if (pcb->get_pos(0, &x, &y) && x == xloc && y == yloc) {
                    if (pn)
                        bit_to_named(pn, 0, NetexWrap(pcb).netex());
                    else if (pb)
                        named_to_cell(NetexWrap(pcb).netex(), pb);
                }
                continue;
            }
        }
        else if (isDevOrSubc(tp)) {
            // A device or subcircuit.

            bool issym = (tp == CDelecDev);
            if (!issym)
                issym = msdesc->isSymbolic();

            CDp_cnode *pcn = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pcn; pcn = pcn->next()) {
                if (in_contact(pcn, issym, xloc, yloc)) {
                    CDp_range *pcr = (CDp_range*)cdesc->prpty(P_RANGE);
                    if (pn)
                        bit_to_bit(pcn, pcr, pn, 0);
                    else if (pb)
                        bit_to_cell(pcn, pcr, pb);
                }
            }

            CDp_bcnode *pcb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
            for ( ; pcb; pcb = pcb->next()) {
                if (in_contact(pcb, issym, xloc, yloc)) {
                    CDp_range *pcr = (CDp_range*)cdesc->prpty(P_RANGE);
                    if (pn)
                        bit_to_inst(pn, 0, cdesc, pcb, pcr);
                    else if (pb)
                        inst_to_cell(cdesc, pcb, pcr, pb);
                }
            }
        }
    }
}


//
// The following xxx_to_yyy() functions extablish the connections
// between two connectable objects.  The code words indicate:
//
//   bit        Single-bit connection to an instance, possibly
//              vectored.
//   wbit       Wire-bit, basically same as bit with no instance
//              vectorization.
//   inst       Multi-pin connection to an instance, possibly
//              vectored.
//   named      A multi-bit bundle or bus, described by a CDnetex.
//              This can be a named wire, terminal device, or cell
//              terminal.
//   cell       Cell multi-contact terminal, can be named or unnamed.
//

void
cScedConnect::bit_to_bit(CDp_nodeEx *pcn1, const CDp_range *pr1,
    CDp_nodeEx *pcn2, const CDp_range *pr2)
{
    if (!pcn1 || !pcn2 || pcn1 == pcn2)
        return;
    if (pr1 && pr1->width() == 1)
        pr1 = 0;
    if (pr2 && pr2->width() == 1)
        pr2 = 0;
    if (!pr1 && !pr2) {
        connect_nodes(pcn1, pcn2);
        return;
    }
    if (pr1 && pr2) {
        int w1 = pr1->width();
        int w2 = pr2->width();
        if (w1 != w2) {
            char *tn1 = pcn1->id_text();
            char *tn2 = pcn2->id_text();
            ScedErrLog.add_err(
                "warning, %s and %s widths %d and %d differ,\n"
                "connecting what I can anyway.", tn1, tn2, w1, w2);
            delete [] tn1;
            delete [] tn2;
            // Not an error, don't return.
        }

        CDgenRange rgen1(pr1);
        CDgenRange rgen2(pr2);
        int cnt = 0;
        while (rgen1.next(0) && rgen2.next(0)) {
            CDp_nodeEx *pn1;
            if (cnt == 0)
                pn1 = pcn1;
            else
                pn1 = pr1->node(0, cnt, pcn1->index());
            CDp_nodeEx *pn2;
            if (cnt == 0)
                pn2 = pcn2;
            else
                pn2 = pr2->node(0, cnt, pcn2->index());
            cnt++;
            connect_nodes(pn1, pn2);
        }
        return;
    }
    if (!pr2) {
        CDp_nodeEx *tp = pcn1; pcn1 = pcn2; pcn2 = tp;
        const CDp_range *tv = pr1; pr1 = pr2; pr2 = tv;
    }
    CDgenRange rgen2(pr2);
    int cnt = 0;
    while (rgen2.next(0)) {
        CDp_nodeEx *pn2;
        if (cnt == 0)
            pn2 = pcn2;
        else
            pn2 = pr2->node(0, cnt, pcn2->index());
        cnt++;
        connect_nodes(pcn1, pn2);
    }
}


void
cScedConnect::bit_to_inst(CDp_nodeEx *pcn1, const CDp_range *pr1,
    const CDc *cdesc2, const CDp_bcnode *pbcn2, const CDp_range *pr2)
{
    if (!pcn1 || !pbcn2)
        return;
    if (pr1 && pr1->width() == 1)
        pr1 = 0;
    if (pr2 && pr2->width() == 1)
        pr2 = 0;
    if (pbcn2->width() == 1) {
        CDp_cnode *pn2 = find_node_prp(cdesc2, pbcn2, 0);
        if (pn2)
            bit_to_bit(pcn1, pr1, pn2, pr2);
        return;
    }

    if (!pr1) {
        if (!pr2) {
            // Scalar, connect to all bits.
            CDgenRange rgen(pbcn2);
            unsigned int indx = 0;
            while (rgen.next(0)) {
                CDp_cnode *pn2 = find_node_prp(cdesc2, pbcn2, indx);
                if (!pn2)
                    break;
                indx++;
                connect_nodes(pcn1, pn2);
            }
        }
        else {
            // Scalar, connect to all bits.
            CDgenRange rgen0(pr2);
            int cnt = 0;
            while (rgen0.next(0)) {
                CDgenRange rgen2(pbcn2);
                int indx = 0;
                while (rgen2.next(0)) {
                    CDp_cnode *pn0 = find_node_prp(cdesc2, pbcn2, indx);
                    if (!pn0)
                        break;

                    CDp_cnode *pn2;
                    if (cnt == 0)
                        pn2 = pn0;
                    else
                        pn2 = pr2->node(0, cnt, pn0->index());
                    indx++;
                    connect_nodes(pcn1, pn2);
                }
                cnt++;
            }
        }
    }
    else {
        if (!pr2) {
            int w1 = pr1->width();
            int w2 = pbcn2->width();
            if (w1 != w2) {
                char *tn1 = pcn1->id_text();
                char *tn2 = pbcn2->id_text();
                ScedErrLog.add_err(
                    "warning, %s and %s widths %d and %d differ,\n"
                    "connecting what I can anyway.", tn1, tn2, w1, w2);
                delete [] tn1;
                delete [] tn2;
                // Not an error, don't return.
            }

            CDgenRange rgen1(pr1);
            CDgenRange rgen2(pbcn2);
            unsigned int indx = 0;
            int cnt = 0;
            while (rgen1.next(0) && rgen2.next(0)) {
                CDp_cnode *pn2 = find_node_prp(cdesc2, pbcn2, indx);
                if (!pn2)
                    break;
                indx++;
                CDp_nodeEx *pn1;
                if (cnt == 0)
                    pn1 = pcn1;
                else
                    pn1 = pr1->node(0, cnt, pcn1->index());
                cnt++;
                connect_nodes(pn1, pn2);
            }
        }
        else {
            int iw1 = pr1->width();
//XXX
            int iw2 = pr2->width() * pbcn2->width();
            if (iw1 != iw2) {
                // We require that the instance vector widths be
                // equal.

                char *tn1 = pcn1->id_text();
                char *tn2 = pbcn2->id_text();
                ScedErrLog.add_err(
                    "can't connect %s and %s/%s, instances have\n"
                    "different vector widths %d and %d.",
                    tn1, cdesc2->getElecInstBaseName(), tn2, iw1, iw2);
                delete [] tn1;
                delete [] tn2;
                return;
            }

#define NEWXXX
#ifdef NEWXXX
            CDgenRange rgen0(pr2);
            int cnt = 0;
            int aindx = 0;
            while (rgen0.next(0)) {
                CDgenRange rgen2(pbcn2);
                int indx = 0;

                while (rgen2.next(0)) {
                    CDp_cnode *pn0 = find_node_prp(cdesc2, pbcn2, aindx);
                    if (!pn0)
                        break;
                    CDp_cnode *pn2;
                    if (indx == 0)
                        pn2 = pn0;
                    else
                        pn2 = pr2->node(0, indx, pn0->index());

                    CDp_nodeEx *pn1;
                    if (cnt == 0)
                        pn1 = pcn1;
                    else
                        pn1 = pr1->node(0, cnt, pcn1->index());
                    indx++;
                    cnt++;
                    connect_nodes(pn1, pn2);
                }
                aindx++;
            }
#else
            CDgenRange rgen0(pr2);
            int cnt = 0;
            while (rgen0.next(0)) {
                CDgenRange rgen2(pbcn2);
                int indx = 0;
                CDp_nodeEx *pn1;
                if (cnt == 0)
                    pn1 = pcn1;
                else
                    pn1 = pr1->node(0, cnt, pcn1->index());

                while (rgen2.next(0)) {
                    CDp_cnode *pn0 = find_node_prp(cdesc2, pbcn2, indx);
                    if (!pn0)
                        break;
                    CDp_cnode *pn2;
                    if (cnt == 0)
                        pn2 = pn0;
                    else
                        pn2 = pr2->node(0, cnt, pn0->index());
                    indx++;
                    connect_nodes(pn1, pn2);
                }
                cnt++;
            }
#endif
        }
    }
}


void
cScedConnect::bit_to_named(CDp_nodeEx *pcn1, const CDp_range *pr1,
    const CDnetex *nx2)
{
    if (!pcn1 || !nx2)
        return;
    if (pr1 && pr1->width() == 1)
        pr1 = 0;
    if (nx2->width() == 1) {
        CDnetexGen ngen(nx2);
        CDnetName name;
        int n;
        if (ngen.next(&name, &n)) {
            name_elt *ne = tname_tab_find(Tstring(name), n);
            if (ne && ne->nodeprp())
                wbit_to_bit(ne->nodeprp(), pcn1, pr1);
        }
        return;
    }

    if (!pr1) {
        // Scalar, connect to all bits.
        CDnetexGen ngen(nx2);
        CDnetName name;
        int n;
        while (ngen.next(&name, &n)) {
            name_elt *ne = tname_tab_find(Tstring(name), n);
            if (!ne || !ne->nodeprp())
                continue;
            connect_nodes(pcn1, ne->nodeprp());
        }
    }
    else {
        int w1 = pr1->width();
        int w2 = nx2->width();
        if (w1 != w2) {
            char *tn1 = pcn1->id_text();
            char *tn2 = CDnetex::id_text(nx2);
            ScedErrLog.add_err(
                "warning, %s and %s widths %d and %d differ,\n"
                "connecting what I can anyway.", tn1, tn2, w1, w2);
            delete [] tn1;
            delete [] tn2;
            // Not an error, don't return.
        }

        CDgenRange rgen1(pr1);
        CDnetexGen ngen2(nx2);
        CDnetName name2;
        int n2;
        int cnt = 0;
        while (rgen1.next(0) && ngen2.next(&name2, &n2)) {
            CDp_nodeEx *pn1;
            if (cnt == 0)
                pn1 = pcn1;
            else
                pn1 = pr1->node(0, cnt, pcn1->index());
            cnt++;
            name_elt *ne = tname_tab_find(Tstring(name2), n2);
            if (!ne || !ne->nodeprp())
                continue;
            CDp_node *pn2 = ne->nodeprp();
            connect_nodes(pn1, pn2);
        }
    }
}


void
cScedConnect::bit_to_cell(CDp_nodeEx *pcn1, const CDp_range *pr1,
    const CDp_bsnode *pbsn2)
{
    if (!pcn1 || !pbsn2)
        return;
    if (pr1 && pr1->width() == 1)
        pr1 = 0;
    if (pbsn2->width() == 1) {
        CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, 0);
        if (pn2)
            bit_to_bit(pcn1, pr1, pn2, 0);
        return;
    }

    if (!pr1) {
        // Scalar, connect to all bits.
        CDgenRange rgen(pbsn2);
        unsigned indx = 0;
        while (rgen.next(0)) {
            CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, indx);
            if (!pn2)
                break;
            indx++;
            connect_nodes(pcn1, pn2);
        }
    }
    else {
        int w1 = pr1->width();
        int w2 = pbsn2->width();
        if (w1 != w2) {
            char *tn1 = pcn1->id_text();
            char *tn2 = pbsn2->id_text();
            ScedErrLog.add_err(
                "warning, %s and %s widths %d and %d differ,\n"
                "connecting what I can anyway.", tn1, tn2, w1, w2);
            delete [] tn1;
            delete [] tn2;
            // Not an error, don't return.
        }

        CDgenRange rgen2(pbsn2);
        unsigned indx = 0;
        CDgenRange rgen1(pr1);
        int cnt = 0;
        while (rgen1.next(0) && rgen1.next(0)) {
            CDp_nodeEx *pn1;
            if (cnt == 0)
                pn1 = pcn1;
            else
                pn1 = pr1->node(0, cnt, pcn1->index());
            cnt++;
            CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, indx);
            if (!pn2)
                break;
            indx++;
            connect_nodes(pn1, pn2);
        }
    }
}


void
cScedConnect::wbit_to_bit(CDp_node *pn1, CDp_nodeEx *pcn2, const CDp_range *pr2)
{
    if (!pn1 || !pcn2)
        return;
    if (pr2 && pr2->width() == 1)
        pr2 = 0;
    if (!pr2)
        connect_nodes(pn1, pcn2);
    else {
        CDgenRange rgen(pr2);
        int cnt = 0;
        while (rgen.next(0)) {
            CDp_nodeEx *pn2;
            if (cnt == 0)
                pn2 = pcn2;
            else
                pn2 = pr2->node(0, cnt, pcn2->index());
            cnt++;
            connect_nodes(pn1, pn2);
        }
    }
}


void
cScedConnect::wbit_to_inst(CDp_node *pn1, const CDc *cdesc2,
    const CDp_bcnode *pbcn2, const CDp_range *pr2)
{
    if (!pn1 || !pbcn2)
        return;
    if (pr2 && pr2->width() == 1)
        pr2 = 0;
    if (pbcn2->width() == 1) {
        CDp_cnode *pn2 = find_node_prp(cdesc2, pbcn2, 0);
        if (pn2)
            wbit_to_bit(pn1, pn2, pr2);
        return;
    }

    if (!pr2) {
        // Scalar, connect to all bits.
        CDgenRange rgen(pbcn2);
        unsigned int indx = 0;
        while (rgen.next(0)) {
            CDp_cnode *pn2 = find_node_prp(cdesc2, pbcn2, indx);
            if (!pn2)
                break;
            indx++;
            connect_nodes(pn1, pn2);
        }
    }
    else {
        CDgenRange rgen0(pr2);
        int cnt = 0;
        while (rgen0.next(0)) {
            CDgenRange rgen2(pbcn2);
            int indx = 0;
            while (rgen2.next(0)) {
                CDp_cnode *pn0 = find_node_prp(cdesc2, pbcn2, indx);
                if (!pn0)
                    break;

                CDp_cnode *pn2;
                if (cnt == 0)
                    pn2 = pn0;
                else
                    pn2 = pr2->node(0, cnt, pn0->index());
                indx++;
                connect_nodes(pn1, pn2);
            }
            cnt++;
        }
    }
}


void
cScedConnect::wbit_to_named(CDp_node *pn1, const CDnetex *nx2)
{
    // Scalar, connect to all bits.
    if (!pn1 || !nx2)
        return;
    CDnetexGen ngen(nx2);
    CDnetName name;
    int n;
    while (ngen.next(&name, &n)) {
        name_elt *ne = tname_tab_find(Tstring(name), n);
        if (!ne || !ne->nodeprp())
            continue;
        connect_nodes(pn1, ne->nodeprp());
    }
}


void
cScedConnect::wbit_to_cell(CDp_node *pn1, const CDp_bsnode *pbsn2)
{
    // Scalar, connect to all bits.
    if (!pn1 || !pbsn2)
        return;
    CDgenRange rgen(pbsn2);
    unsigned indx = 0;
    while (rgen.next(0)) {
        CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, indx);
        if (!pn2)
            break;
        indx++;
        connect_nodes(pn1, pn2);
    }
}


void
cScedConnect::inst_to_inst(const CDc *cdesc1, const CDp_bcnode *pbcn1,
    const CDp_range *pr1, const CDc *cdesc2, const CDp_bcnode *pbcn2,
    const CDp_range *pr2)
{
    if (!pbcn1 || !pbcn2 || pbcn1 == pbcn2)
        return;
    if (pr1 && pr1->width() == 1)
        pr1 = 0;
    if (pr2 && pr2->width() == 1)
        pr2 = 0;
    if (pbcn1->width() == 1) {
        CDp_cnode *pn1 = find_node_prp(cdesc1, pbcn1, 0);
        if (pn1)
            bit_to_inst(pn1, pr1, cdesc2, pbcn2, pr2);
        return;
    }
    if (pbcn2->width() == 1) {
        CDp_cnode *pn2 = find_node_prp(cdesc2, pbcn2, 0);
        if (pn2)
            bit_to_inst(pn2, pr2, cdesc1, pbcn1, pr1);
        return;
    }

    if (!pr1 && !pr2) {
        int w1 = pbcn1->width();
        int w2 = pbcn2->width();
        if (w1 != w2) {
            char *tn1 = pbcn1->id_text();
            char *tn2 = pbcn2->id_text();
            ScedErrLog.add_err(
                "warning, %s and %s widths %d and %d differ,\n"
                "connecting what I can anyway.", tn1, tn2, w1, w2);
            delete [] tn1;
            delete [] tn2;
            // Not an error, don't return.
        }

        CDgenRange rgen1(pbcn1);
        CDgenRange rgen2(pbcn2);
        unsigned int n1, n2;
        unsigned int indx1 = 0;
        unsigned int indx2 = 0;
        while (rgen1.next(&n1) && rgen2.next(&n2)) {
            CDp_cnode *pn1 = find_node_prp(cdesc1, pbcn1, indx1);
            if (!pn1)
                break;
            indx1++;
            CDp_cnode *pn2 = find_node_prp(cdesc2, pbcn2, indx2);
            if (!pn2)
                break;
            indx2++;
            connect_nodes(pn1, pn2);
        }
        return;
    }
    if (pr1 && pr2) {
        int cw1 = pbcn1->width();
        int iw1 = pr1->width();
        int cw2 = pbcn2->width();
        int iw2 = pr2->width();
        if (cw1 != cw2 || iw1 != iw2) {
            // We require these to match.
            char *tn1 = pbcn1->id_text();
            char *tn2 = pbcn2->id_text();
            ScedErrLog.add_err(
                "can't connect %s/%s and %s/%s, instances have\n"
                "vector widths %d and %d, connector widths are %d and %d,\n"
                "these must match.",
                cdesc1->getElecInstBaseName(), tn1,
                cdesc2->getElecInstBaseName(), tn2,
                iw1, iw2, cw1, cw2);
            delete [] tn1;
            delete [] tn2;
            return;
        }

        CDgenRange rgen0(pr1);
        int cnt = 0;
        while (rgen0.next(0)) {
            CDgenRange rgen1(pbcn1);
            int indx1 = 0;
            CDgenRange rgen2(pbcn2);
            int indx2 = 0;
            while (rgen1.next(0) && rgen2.next(0)) {
                CDp_cnode *pn0 = find_node_prp(cdesc1, pbcn1, indx1);
                if (!pn0)
                    break;
                CDp_cnode *pn1;
                if (cnt == 0)
                    pn1 = pn0;
                else
                    pn1 = pr1->node(0, cnt, pn0->index());
                indx1++;
                pn0 = find_node_prp(cdesc2, pbcn2, indx2);
                if (!pn0)
                    break;
                CDp_cnode *pn2;
                if (cnt == 0)
                    pn2 = pn0;
                else
                    pn2 = pr2->node(0, cnt, pn0->index());
                indx2++;
                connect_nodes(pn1, pn2);
            }
            cnt++;
        }
    }
    if (!pr1) {
        // Swap sets, so pr2 will be zero.
        const CDc *tc = cdesc1; cdesc1 = cdesc2; cdesc2 = tc;
        const CDp_bcnode *tpb = pbcn1; pbcn1 = pbcn2; pbcn2 = tpb;
        const CDp_range *tpr = pr1; pr1 = pr2; pr2 = tpr;
    }
    int bw = pbcn2->width();
    int cw = pbcn1->width();
    int iw = pr1->width();
    bool roll = (bw == cw);
    if (!roll && bw != iw*cw) {
        // We require bw == cw or bw == iw*cw.
        char *tn1 = pbcn1->id_text();
        char *tn2 = pbcn2->id_text();
        ScedErrLog.add_err(
            "can't connect %s/%s and %s/%s, instances have\n"
            "vector widths %d and 1, connector widths are %d and %d,\n"
            "scalar width must equal connector or total width.",
            cdesc1->getElecInstBaseName(), tn1,
            cdesc2->getElecInstBaseName(), tn2,
            iw, cw, bw);
        delete [] tn1;
        delete [] tn2;
        return;
    }

    CDgenRange rgen0(pr1);
    int cnt = 0;
    CDgenRange rgen2(pbcn2);
    int indx2 = 0;
    while (rgen0.next(0)) {
        CDgenRange rgen1(pbcn1);
        int indx1 = 0;
        while (rgen1.next(0)) {
            CDp_cnode *pn0 = find_node_prp(cdesc1, pbcn1, indx1);
            if (!pn0)
                break;
            CDp_cnode *pn1;
            if (cnt == 0)
                pn1 = pn0;
            else
                pn1 = pr1->node(0, cnt, pn0->index());
            indx1++;
            CDp_node *pn2 = find_node_prp(cdesc2, pbcn2, indx2);
            if (!pn2) {
                if (!roll)
                    return;
                indx2 = 0;
                pn2 = find_node_prp(cdesc2, pbcn2, indx2);
            }
            indx2++;
            connect_nodes(pn1, pn2);
        }
        cnt++;
    }
}


void
cScedConnect::inst_to_named(const CDc *cdesc, const CDp_bcnode *pbcn1,
    const CDp_range *pr1, const CDnetex *nx2)
{
    if (!pbcn1 || ! nx2)
        return;
    if (pr1 && pr1->width() == 1)
        pr1 = 0;
    if (pbcn1->width() == 1) {
        CDp_cnode *pn1 = find_node_prp(cdesc, pbcn1, 0);
        if (pn1)
            bit_to_named(pn1, pr1, nx2);
        return;
    }
    if (nx2->width() == 1) {
        CDnetexGen ngen2(nx2);
        CDnetName name2;
        int n2;
        if (ngen2.next(&name2, &n2)) {
            name_elt *ne = tname_tab_find(Tstring(name2), n2);
            if (ne && ne->nodeprp())
                wbit_to_inst(ne->nodeprp(), cdesc, pbcn1, pr1);
        }
        return;
    }

    if (!pr1) {
        int w1 = pbcn1->width();
        int w2 = nx2->width();
        if (w1 != w2) {
            int m = nx2->multip();
            if (m == 1 || w1 != w2/m) {
                char *tn1 = pbcn1->id_text();
                char *tn2 = CDnetex::id_text(nx2);
                ScedErrLog.add_err(
                    "warning, %s and %s widths %d and %d differ,\n"
                    "connecting what I can anyway.", tn1, tn2, w1, w2);
                delete [] tn1;
                delete [] tn2;
                // Not an error, don't return.
            }
        }

        CDgenRange rgen1(pbcn1);
        unsigned int indx = 0;
        CDnetexGen ngen2(nx2);
        CDnetName name2;
        int n2;
        while (rgen1.next(0) && ngen2.next(&name2, &n2)) {
            CDp_cnode *pn1 = find_node_prp(cdesc, pbcn1, indx);
            if (!pn1)
                break;
            indx++;
            name_elt *ne = tname_tab_find(Tstring(name2), n2);
            if (!ne || !ne->nodeprp())
                continue;
            CDp_node *pn2 = ne->nodeprp();
            connect_nodes(pn1, pn2);
        }
    }
    else {
        int bw = nx2->width();
        int m = nx2->multip();
        int cw = pbcn1->width();
        int iw = pr1->width();
        bool roll = (bw/m == cw);
        if (!roll && bw != iw*cw) {
            // We require bw/m == cw or bw == iw*cw.
            char *tn1 = pbcn1->id_text();
            char *tn2 = CDnetex::id_text(nx2);
            ScedErrLog.add_err(
                "can't connect %s/%s and %s, instances have\n"
                "vector widths %d and 1, connector widths are %d and %d,\n"
                "scalar width must equal connector or total width.",
                cdesc->getElecInstBaseName(), tn1, tn2, iw, cw, bw);
            delete [] tn1;
            delete [] tn2;
            return;
        }

        CDnetexGen ngen(nx2);
        CDgenRange rgen0(pr1);
        int cnt = 0;
        while (rgen0.next(0)) {
            CDgenRange rgen1(pbcn1);
            int indx = 0;
            while (rgen1.next(0)) {
                CDp_cnode *pn0 = find_node_prp(cdesc, pbcn1, indx);
                if (!pn0)
                    break;
                CDp_cnode *pn1;
                if (cnt == 0)
                    pn1 = pn0;
                else
                    pn1 = pr1->node(0, cnt, pn0->index());
                indx++;
                CDnetName nm;
                int n;
                if (!ngen.next(&nm, &n)) {
                    if (!roll)
                        return;
                    ngen = CDnetexGen(nx2);
                    ngen.next(&nm, &n);
                }
                name_elt *ne = tname_tab_find(Tstring(nm), n);
                if (!ne || !ne->nodeprp())
                    continue;
                connect_nodes(pn1, ne->nodeprp());
            }
            cnt++;
        }
    }
}


void
cScedConnect::inst_to_cell(const CDc *cdesc, const CDp_bcnode *pbcn1,
    const CDp_range *pr1, const CDp_bsnode *pbsn2)
{
    if (!pbcn1 || ! pbsn2)
        return;
    if (pr1 && pr1->width() == 1)
        pr1 = 0;
    if (pbcn1->width() == 1) {
        CDp_cnode *pn1 = find_node_prp(cdesc, pbcn1, 0);
        if (pn1)
            bit_to_cell(pn1, pr1, pbsn2);
        return;
    }
    if (pbsn2->width() == 1) {
        CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, 0);
        if (pn2)
            bit_to_inst(pn2, 0, cdesc, pbcn1, pr1);
        return;
    }

    if (!pr1) {
        int w1 = pbcn1->width();
        int w2 = pbsn2->width();
        if (w1 != w2) {
            char *tn1 = pbcn1->id_text();
            char *tn2 = pbsn2->id_text();
            ScedErrLog.add_err(
                "warning, %s and %s widths %d and %d differ,\n"
                "connecting what I can anyway.", tn1, tn2, w1, w2);
            delete [] tn1;
            delete [] tn2;
            // Not an error, don't return.
        }

        CDgenRange rgen1(pbcn1);
        unsigned int indx1 = 0;
        CDgenRange rgen2(pbsn2);
        unsigned int indx2 = 0;
        while (rgen1.next(0) && rgen2.next(0)) {
            CDp_cnode *pn1 = find_node_prp(cdesc, pbcn1, indx1);
            if (!pn1)
                break;
            indx1++;
            CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, indx2);
            if (!pn2)
                break;
            indx2++;
            connect_nodes(pn1, pn2);
        }
    }
    else {
        int bw = pbsn2->width();
        int cw = pbcn1->width();
        int iw = pr1->width();
        bool roll = (bw == cw);
        if (!roll && bw != iw*cw) {
            // We require bw == cw or bw == iw*cw.
            char *tn1 = pbcn1->id_text();
            char *tn2 = pbsn2->id_text();
            ScedErrLog.add_err(
                "can't connect %s/%s and cell %s, instance has\n"
                "vector width %d, connector widths are %d and %d,\n"
                "scalar width must equal connector or total width.",
                cdesc->getElecInstBaseName(), tn1, tn2, iw, cw, bw);
            delete [] tn1;
            delete [] tn2;
            return;
        }
        CDgenRange rgen0(pr1);
        int cnt = 0;
        CDgenRange rgen2(pbsn2);
        unsigned int indx2 = 0;
        while (rgen0.next(0)) {
            CDgenRange rgen1(pbcn1);
            int indx = 0;
            while (rgen1.next(0)) {
                CDp_cnode *pn0 = find_node_prp(cdesc, pbcn1, indx);
                if (!pn0)
                    break;
                CDp_cnode *pn1;
                if (cnt == 0)
                    pn1 = pn0;
                else
                    pn1 = pr1->node(0, cnt, pn0->index());
                indx++;

                CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, indx2);
                if (!pn2) {
                    if (!roll)
                        return;
                    indx2 = 0;
                    pn2 = find_node_prp(cn_sdesc, pbsn2, indx2);
                    if (!pn2)
                        return;
                }
                indx2++;
                connect_nodes(pn1, pn2);
            }
            cnt++;
        }
    }
}


void
cScedConnect::named_to_named(const CDnetex *nx1, const CDnetex *nx2)
{
    if (!nx1 || !nx2 || nx1 == nx2)
        return;

    if (!CDnetex::check_compatible(nx1, nx2)) {
        char *tn1 = CDnetex::id_text(nx1);
        char *tn2 = CDnetex::id_text(nx2);
        ScedErrLog.add_err(
            "warning: incompatible expressions in connected named subnets.\n"
            "Expressions are %s and %s.", tn1, tn2);
        delete [] tn1;
        delete [] tn2;
    }
}


void
cScedConnect::named_to_cell(const CDnetex *nx1, const CDp_bsnode *pbsn2)
{
    if (!nx1 || !pbsn2)
        return;
    int w1 = nx1->width();
    if (w1 == 1) {
        CDnetexGen ngen1(nx1);
        CDnetName name1;
        int n1;
        if (ngen1.next(&name1, &n1)) {
            name_elt *ne1 = tname_tab_find(Tstring(name1), n1);
            if (ne1 && ne1->nodeprp())
                wbit_to_cell(ne1->nodeprp(), pbsn2);
        }
        return;
    }
    int w2 = pbsn2->width();
    if (w2 == 1) {
        CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, 0);
        if (pn2)
            bit_to_named(pn2, 0, nx1);
        return;
    }

    if (w1 != w2) {
        char *tn1 = CDnetex::id_text(nx1);
        char *tn2 = pbsn2->id_text();
        ScedErrLog.add_err(
            "warning, %s and %s widths %d and %d differ,\n"
            "connecting what I can anyway.", tn1, tn2,
            w1, w2);
        delete [] tn1;
        delete [] tn2;
        // Not an error, don't return.
    }

    CDnetexGen ngen1(nx1);
    CDnetName name1;
    int n1;
    CDgenRange rgen2(pbsn2);
    unsigned int indx = 0;
    while (ngen1.next(&name1, &n1) && rgen2.next(0)) {
        name_elt *ne1 = tname_tab_find(Tstring(name1), n1);
        if (!ne1 || !ne1->nodeprp())
            continue;
        CDp_node *pn1 = ne1->nodeprp();
        CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, indx);
        if (!pn2)
            break;
        indx++;
        connect_nodes(pn1, pn2);
    }
}


// This probably makes no sense, two co-located cell terminals.
//
void
cScedConnect::cell_to_cell(const CDp_bsnode *pbsn1, const CDp_bsnode *pbsn2)
{
    if (!pbsn1 || !pbsn2 || pbsn1 == pbsn2)
        return;
    if (pbsn1->width() == 1) {
        CDp_snode *pn1 = find_node_prp(cn_sdesc, pbsn1, 0);
        if (pn1)
            bit_to_cell(pn1, 0, pbsn2);
        return;
    }
    if (pbsn2->width() == 1) {
        CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, 0);
        if (pn2)
            bit_to_cell(pn2, 0, pbsn1);
        return;
    }

    CDgenRange rgen1(pbsn1);
    unsigned int indx1 = 0;
    CDgenRange rgen2(pbsn2);
    unsigned int indx2 = 0;
    while (rgen1.next(0) && rgen2.next(0)) {
        CDp_snode *pn1 = find_node_prp(cn_sdesc, pbsn1, indx1);
        if (!pn1)
            break;
        indx1++;
        CDp_snode *pn2 = find_node_prp(cn_sdesc, pbsn2, indx2);
        if (!pn2)
            break;
        indx2++;
        connect_nodes(pn1, pn2);
    }
}


void
cScedConnect::connect_nodes(CDp_node *pn1, CDp_node *pn2)
{

    int node1 = pn1->enode();
    int node2 = pn2->enode();

    ScedErrLog.add_log("connect %d to %d", node1, node2);

    if (node1 < 0 && node2 < 0) {
        add_to_ntab(cn_count, pn1);
        add_to_ntab(cn_count, pn2);
        new_node();
    }
    else if (node1 < 0)
        add_to_ntab(node2, pn1);
    else if (node2 < 0)
        add_to_ntab(node1, pn2);
    else 
        merge(node1, node2);
}


// Check the bus terminal bit order, create map if necessary.
//
void
cScedConnect::setup_map(CDp_bsnode *pb)
{
    if (!pb->has_name()) {
        // An unnamed terminal can't find scalar terms by name, so
        // there will never be a map.
        pb->set_index_map(0);
        return;
    }
    int width = pb->width();
    unsigned short *map = new unsigned short[width];
    CDnetexGen ngen(pb);
    CDnetName nm;
    int n;
    unsigned int indx = 0;
    bool need_map = false;
    while (ngen.next(&nm, &n)) {

        CDp_snode *ps = (CDp_snode*)cn_sdesc->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            if (!ps->get_term_name())
                continue;
            CDnetName tnm;
            int tn;
            if (!CDnetex::parse_bit(Tstring(ps->get_term_name()),
                    &tnm, &tn)) {
                Errs()->get_error();
                continue;
            }
            if (nm == tnm && n == tn)
                break;
        }
        if (!ps) {
            CDnetName tnm = CDnetex::mk_name(Tstring(nm), n);
            ScedErrLog.add_err("can't find terminal %s for bus terminal %s",
                Tstring(tnm), pb->term_name());
            need_map = true;
            map[indx] = P_NODE_MAX_INDEX + 1;
        }
        else {
            need_map = (ps->index() != pb->index() + indx);
            map[indx] = ps->index();
        }
        indx++;
    }
    if (need_map) {
        ScedErrLog.add_log("using index mapping for %s", pb->term_name());
        pb->set_index_map(map);
    }
    else {
        pb->set_index_map(0);
        delete [] map;
    }
}


// Increment the node count, expand list array when necessary.
//
void
cScedConnect::new_node()
{
    cn_count++;
    if (!cn_ntab) {
        cn_ntab = new node_list*[INIT_NTSIZE];
        memset(cn_ntab, 0, INIT_NTSIZE*sizeof(node_list*));
        cn_ntsize = INIT_NTSIZE;
    }
    if (cn_count >= cn_ntsize) {
        node_list **tmpnt = new node_list*[cn_ntsize + cn_ntsize];
        int i;
        for (i = 0; i < cn_ntsize; i++)
            tmpnt[i] = cn_ntab[i];
        cn_ntsize += cn_ntsize;
        for ( ; i < cn_ntsize; i++)
            tmpnt[i] = 0;
        delete [] cn_ntab;
        cn_ntab = tmpnt;
    }
}


// Add pn to the list for n, set the node.
//
void
cScedConnect::add_to_ntab(int n, CDp_node *pn)
{
    if (n < 0 || n > cn_count)
        return;
    if (!pn)
        return;
    pn->set_enode(n);
    cn_ntab[n] = new node_list(pn, cn_ntab[n]);
}


void
cScedConnect::add_to_tname_tab(name_elt *ne)
{
    if (!ne)
        return;
    if (cn_case_insens) {
        cn_u.tname_tab_ci->link(ne);
        cn_u.tname_tab_ci = cn_u.tname_tab_ci->check_rehash();
    }
    else {
        cn_u.tname_tab->link(ne);
        cn_u.tname_tab = cn_u.tname_tab->check_rehash();
    }
}


cScedConnect::name_elt *
cScedConnect::tname_tab_find(const char *name, int n)
{
    if (!cn_u.tname_tab || !name)
        return (0);
    if (cn_case_insens)
        return (cn_u.tname_tab_ci->find(name, n));
    return (cn_u.tname_tab->find(name, n));
}


// Combine the two nets into the smaller of n1,n2.  The larger is left
// empty.
//
void
cScedConnect::merge(int n1, int n2)
{
    if (n1 < 0 || n1 >= cn_count)
        return;
    if (n2 < 0 || n2 >= cn_count)
        return;
    if (n1 == n2)
        return;

    if (ScedErrLog.log_connect()) {
        for (node_list *n = cn_ntab[n1]; n; n = n->next()) {
            if (n->node()->enode() != n1)
                ScedErrLog.add_err("node table merge left, incorrect node "
                "number %d in %d list.", n->node()->enode(), n1);
        }
        for (node_list *n = cn_ntab[n2]; n; n = n->next()) {
            if (n->node()->enode() != n2)
                ScedErrLog.add_err("node table merge right, incorrect node "
                "number %d in %d list.", n->node()->enode(), n2);
        }
    }

    if (n2 < n1) {
        int t = n2;
        n2 = n1;
        n1 = t;
    }

    node_list *n0 = cn_ntab[n2];
    if (!n0)
        return;
    cn_ntab[n2] = 0;

    node_list *n = n0;
    for (;;) {
        n->node()->set_enode(n1);
        if (!n->next())
            break;
        n = n->next();
    }
    n->set_next(cn_ntab[n1]);
    cn_ntab[n1] = n0;
}


// Make the numbering compact, starting with nstart.
//
void
cScedConnect::reduce(int nstart)
{
    if (nstart < 0)
        nstart = 0;
    if (nstart >= cn_count)
        return;

    // First, shift down to fill gaps.
    for (int i = nstart; i < cn_count; i++) {
        if (!cn_ntab[i]) {
            for (int j = i+1; j < cn_count; j++)
                cn_ntab[j-1] = cn_ntab[j];
            cn_count--;
            cn_ntab[cn_count] = 0;
            i--;
        }
    }

    // Now renumber everything.
    for (int i = nstart; i < cn_count; i++) {
        if (!cn_ntab[i]) {
            cn_count = i+1;
            break;
        }
        for (node_list *n = cn_ntab[i]; n; n = n->next())
            n->node()->set_enode(i);
    }
    ScedErrLog.add_log("final reduced size %d", cn_count);
}


// Find a node value by name, by searching through the terminal
// devices and named wires.  If a match is found, return true with the
// node in nret.
//
bool
cScedConnect::find_node(const char *tname, int *nret)
{
    if (nret)
        *nret = -1;
    if (!tname)
        return (false);

    int n;
    CDnetName nm;
    if (!CDnetex::parse_bit(tname, &nm, &n))
        return (false);
    name_elt *ne = tname_tab_find(Tstring(nm), n);
    if (!ne)
        return (false);
    *nret = ne->nodeprp()->enode();
    return (true);
}


// Return the node number of the scalar terminal or vertex at the given
// coordinates.  If not found, return -1.
//
int
cScedConnect::find_node(int x, int y)
{
    // Check wire vertices.
    BBox BB(x, y, x, y);
    CDg gdesc;
    CDsLgen gen(cn_sdesc);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(cn_sdesc, ld, &BB);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->type() != CDWIRE)
                continue;
            CDp_node *pn = (CDp_node*)odesc->prpty(P_NODE);
            if (!pn || pn->enode() < 0)
                continue;
            if (((CDw*)odesc)->has_vertex_at(Point_c(x, y)))
                return (pn->enode());
        }
    }

    // Check the dummy node-mapping properties.
    for (CDp_cnode *pn = cn_ndprps; pn; pn = pn->next()) {
        int xx, yy;
        if (!pn->get_pos(0, &xx, &yy))
            continue;
        if (xx == x && yy == y)
            return (pn->enode());
    }

    // Check device, subcircuit, and terminal nodes.
    gdesc.init_gen(cn_sdesc, CellLayer(), &BB);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            continue;

        CDelecCellType tp = msdesc->elecCellType();
        if (tp == CDelecGnd) {
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            if (pn) {
                // A ground device.
                int xx, yy;
                if (pn->get_pos(0, &xx, &yy) && xx == x && yy == y)
                    return (0);
            }
        }
        else if (tp == CDelecTerm) {
            // A scalar terminal.
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            if (pn) {
                int xx, yy;
                if (pn->get_pos(0, &xx, &yy) && xx == x && yy == y)
                    return (pn->enode());
            }
        }
        else if (isDevOrSubc(tp)) {
            // A device or subcircuit.
            if (!cdesc->prpty(P_RANGE)) {

                bool issym = (tp == CDelecDev);
                if (!issym)
                    issym = msdesc->isSymbolic();

                CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
                if (pr)
                    continue;
                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    if (in_contact(pn, issym, x, y))
                        return (pn->enode());
                }
            }
        }
    }
    ScedErrLog.add_log("find_node unresolved at %d, %d.", x, y);
    return (-1);
}

