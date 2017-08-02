
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
#include "fio.h"
#include "cd_celldb.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_lgen.h"
#include "oa_if.h"
#include "oa.h"
#include "oa_net.h"
#include "oa_prop.h"
#include "oa_errlog.h"
#include "symtab.h"


// This sets up ordering of the ports and terminals (terminals can
// connect to multiple ports, i.e., have non-unit width).
//
bool
sOAportTab::port_setup(const oaBlock *block, const cOAelecInfo *cdf)
{
    if (!cdf)
        return (false);
    const char *const *terms = cdf->ports();
    if (!terms)
        terms = cdf->terms();
    if (!terms)
        return (false);

    int pnum = 0;
    int tnum = 0;
    for ( ; *terms; terms++) {
        // NOTE:  The CDBA namespace must be used to handle the
        // angle-bracket subscripts used in Virtuoso.  The CDF
        // source is assumed to use Cadence syntax.

        oaName netname(oaCdbaNS(), *terms);
        oaNet *net = oaNet::find(block, netname);
        if (!net) {
            OAerrLog.add_log(OAlogNet, "net not found: %s.", *terms);
            continue;
        }
        if (net->isGlobal())
            continue;
        oaString nstr;
        netname.get(oaCdbaNS(), nstr);

        add(nstr, tnum, pnum);
        OAerrLog.add_log(OAlogNet, "adding port %s tnum=%d pnum=%d.",
            (const char*)nstr, tnum, pnum);
        tnum++;
        pnum += net->getNumBits();
    }

    if (tnum) {
        // Look through the terminals.  If we find any not listed
        // in our tables, add the names at the end of the order. 
        // This semi-accommodates bad CDF data.

        oaIter<oaTerm> iter(block->getTerms());
        while (oaTerm *term = iter.getNext()) {
            oaNet *net = term->getNet();
            if (net->isGlobal())
                continue;
            oaString termname;
            term->getName(oaCdbaNS(), termname);

            if (!find((const char*)termname)) {
                add(termname, tnum, pnum);
                OAerrLog.add_log(OAlogNet,
                    "adding (not in CDF) %s tnum=%d pnum=%d.",
                    (const char*)termname, tnum, pnum);
                tnum++;
                pnum += net->getNumBits();
            }
        }
    }
    return (true);
}


bool
sOAportTab::find(const char *str, unsigned int *t, unsigned int *p) const
{
    if (!str || !pt_tab)
        return (false);
    CDnetex *nx;
    if (!CDnetex::parse(str, &nx)) {
        // Parse failed, keep it as a scalar.
        Errs()->get_error();
        unsigned long l = (unsigned long)SymTab::get(pt_tab, str);
        if (l == (unsigned long)ST_NIL)
            return (false);
        if (p)
            *p = (l >> 16) & 0xffff;
        if (t)
            *t = l & 0xffff;
    }
    else {
        sLstr lstr;
        CDnetex::print_all(nx, &lstr);
        CDnetex::destroy(nx);
        unsigned long l = (unsigned long)SymTab::get(pt_tab, lstr.string());
        if (l == (unsigned long)ST_NIL)
            return (false);
        if (p)
            *p = (l >> 16) & 0xffff;
        if (t)
            *t = l & 0xffff;
    }
    return (true);
}


void
sOAportTab::add(const char *str, unsigned int t, unsigned int p)
{
    if (!str)
        return;
    if (!pt_tab)
        pt_tab = new SymTab(true, false);
    unsigned long l = ((p & 0xffff) << 16) | (t & 0xffff);
    CDnetex *nx;
    if (!CDnetex::parse(str, &nx) || !nx) {
        Errs()->get_error();
        pt_tab->add(lstring::copy(str), (void*)l, false);
    }
    else {
        sLstr lstr;
        CDnetex::print_all(nx, &lstr);
        CDnetex::destroy(nx);
        pt_tab->add(lstr.string_trim(), (void*)l, false);
    }
}


// Static function.
// SPICE terminal ordering for some common devices.  We shouldn't
// need these, as the terminal ordering should be obtained from
// the OA database somehow.  Terminal ordering for cells
// originating from Xic is set by the P_NODE properties.
//
int
sOAportTab::def_order(const char *tname, int key)
{
    if (isupper(key))
        key = tolower(key);
    int n = tname[0];
    if (isupper(n))
        n = tolower(n);
    if (key == 'm') {
        // mosfet
        switch (n) {
        case 'd': return (0);
        case 'g': return (1);
        case 's': return (2);
        case 'b': return (3);
        default: break;
        }
    }
    else if (key == 'q') {
        // bjt
        switch (n) {
        case 'c': return (0);
        case 'b': return (1);
        case 'e': return (2);
        case 's': return (3);
        default: break;
        }
    }
    return (-1);
}
// End of sOAportTab functions.


// Set up P_NAME, P_NODE, P_NODMAP properties.
//
bool
cOAnetHandler::setupNets(bool symbolic)
{
    if (!nh_block || !nh_sdesc || !nh_sdesc->isElectrical())
        return (true);

    bool ret = true;
    try {
        // Look up the CDF info for this device (if any).
        oaDesign *design = nh_block->getDesign();
        oaLib *lib = design->getLib();
        oaScalarName cellName;
        design->getCellName(cellName);
        oaCell *cell = oaCell::find(lib, cellName);
        const cOAelecInfo *cdf = cOAprop::getCDFinfo(cell, nh_def_symbol,
            nh_def_dev_prop);

        sOAportTab port_tab;
        port_tab.port_setup(nh_block, cdf);

        // First set a P_NAME property if needed.
        CDp_name *pname = (CDp_name*)nh_sdesc->prpty(P_NAME);
        if (!pname) {
            // The device lacks an instNamePrefix property.

            // This could be a "gnd" device, from Xic.  If so, it will
            // have exactly one node property and no subcells.
            CDp_node *pnd = (CDp_node*)nh_sdesc->prpty(P_NODE);
            if (pnd && !pnd->next() && !nh_sdesc->masters()) {
                // Is a gnd device, nothing more to do.
                return (true);
            }
            if (cdf && cdf->prefix()) {
                const char *pfx = cdf->prefix();
                pname = new CDp_name;
                pname->set_name_string(pfx);
                if (*pfx == 'X' || *pfx == 'x')
                    pname->set_subckt(true);
                pname->set_next_prp(nh_sdesc->prptyList());
                nh_sdesc->setPrptyList(pname);
            }
            else {
                // Assume a subcircuit here.
                pname = new CDp_name;
                pname->set_name_string("X");
                pname->set_subckt(true);
                pname->set_next_prp(nh_sdesc->prptyList());
                nh_sdesc->setPrptyList(pname);
            }
        }
        if (!pname->name_string() || pname->key() == P_NAME_NULL) {
            // A "null" device, no nodes.
            return (true);
        }
        int key = pname->key();

        // If the "subcircuit" has exactly one pin, and no instances,
        // assume that it is a terminal device.  It is probably one of
        // the analogLib terminals, which on import will behave like
        // an Xic terminal.  Note that the "gnd" device from analogLib
        // is actually a terminal connected to "gnd!", and NOT an Xic
        // gnd device.

        if (key == 'x' || key == 'X') {
            int numpins = 0;
            oaIter<oaPin> pin_iter(nh_block->getPins());
            while (pin_iter.getNext() != 0)
                numpins++;
            if (numpins == 1) {
                oaDesign *design = nh_block->getDesign();
                oaScalarName cellName;
                design->getCellName(cellName);
                oaString cn;
                cellName.get(cn);

                // Here's a hack, make the "noConn" device not
                // electrically active.

                bool nulldev = (cn == "noConn");

                if (symbolic) {
                    // If there is no schematic view, the cell is a
                    // terminal.  Otherwise, the schematic would have
                    // been checked already as below (schematic view
                    // is read first).

                    oaScalarName libName;
                    oaScalarName viewName(oaNativeNS(), "schematic");
                    design->getLibName(libName);
                    if (!oaDesign::exists(libName, cellName, viewName)) {
                        pname->set_subckt(false);
                        char bf[2];
                        bf[0] = nulldev ? P_NAME_NULL : P_NAME_TERM;
                        bf[1] = 0;
                        pname->set_name_string(bf);
                        key = nulldev ? P_NAME_NULL : P_NAME_TERM;
                    }
                }
                else {
                    int instcnt = 0;
                    oaIter<oaInst> inst_iter(nh_block->getInsts());
                    while (inst_iter.getNext() != 0)
                        instcnt++;

                    if (instcnt == 0) {
                        pname->set_subckt(false);
                        char bf[2];
                        bf[0] = nulldev ? P_NAME_NULL : P_NAME_TERM;
                        bf[1] = 0;
                        pname->set_name_string(bf);
                        key = nulldev ? P_NAME_NULL : P_NAME_TERM;
                    }
                }
            }
        }

        // Now deal with P_NODE and P_BNODE properties.

        int npin = 0;
        int nterm = 0;

        // The terminals are the logical connecters to a cell.
        oaIter<oaTerm> iter(nh_block->getTerms());
        while (oaTerm *term = iter.getNext()) {

            oaString tname;
            term->getName(oaCdbaNS(), tname);

            // If this is a terminal device, and there is no
            // associated label, set the label text field of the
            // property.  When a label is created and associated, it
            // will have this text.
            //
            if (key == P_NAME_TERM && !pname->bound())
                pname->set_label_text(tname);

            unsigned int nbits = term->getNumBits();
            // All terminals have a net (which may be empty).
            oaNet *net = term->getNet();

            unsigned int index_base = 0;
            if (net->isGlobal()) {
                // Save the global name.

                oaString net_name;
                net->getName(oaNativeNS(), net_name);
                ScedIf()->registerGlobalNetName(net_name);
            }
            else {
                unsigned int t, p;
                if (port_tab.find((const char*)tname, &t, &p))
                    index_base = p;
                else {
                    OAerrLog.add_log(OAlogNet,
                        "name %s unresolved in port table.",
                        (const char*)tname);
                    int tnum = sOAportTab::def_order(tname, key);
                    if (tnum > 0)
                        index_base = tnum;
                    else
                        index_base = npin;
                }
            }

            // Each terminal will have pins, which specify contact
            // locations.
            unsigned int pincnt = 0;
            oaIter<oaPin> iter(term->getPins());
            while (oaPin *pin = iter.getNext()) {

                int x, y;
                find_pin_coords(pin, key, symbolic, &x, &y);
                // Note that if find_pin_coords fails, we forge ahead,
                // the pin will be placed at 0,0.

                // If the cell is not a terminal and the pin is
                // connected to a global net, we don't want to create
                // a cell terminal.  Instead, add an Xic terminal at
                // the point, which enforces the node naming and
                // connectivity if the net is split.
                //
                if (net->isGlobal() && key != P_NAME_TERM) {
                    add_terminal(x, y, tname);
                    continue;
                }

                if (term->getType() == oacBusTermType ||
                        term->getType() == oacBundleTermType) {

                    CDnetex *netex;
                    if (!CDnetex::parse(tname, &netex)) {
                        OAerrLog.add_err(IFLOG_WARN,
                            "parse error for %s.\n%s", (const char*)tname,
                            Errs()->get_error());
                        continue;
                    }

                    CDp_bsnode *pb = (CDp_bsnode*)nh_sdesc->prpty(P_BNODE);
                    while (pb) {
                        if (CDnetex::cmp(netex, NetexWrap(pb).netex())) {
                            CDnetex::destroy(netex);
                            break;
                        }
                        pb = pb->next();
                    }
                    if (!pb) {
                        pb = new CDp_bsnode;
                        pb->update_bundle(netex);
                        pb->set_next_prp(nh_sdesc->prptyList());
                        nh_sdesc->setPrptyList(pb);

                        pb->set_index(index_base);

                        // This will be bogus if the bits are
                        // individually placed.  Look at the first
                        // bit, if found in the CDF use its port
                        // number.

                        CDnetexGen ngen(pb);
                        CDnetName nm;
                        int n;
                        if (ngen.next(&nm, &n)) {
                            CDnetName nn = CDnetex::mk_name(Tstring(nm), n);
                            unsigned int t, p;
                            if (port_tab.find(Tstring(nn), &t, &p))
                                pb->set_index(p);
                        }
                        OAerrLog.add_log(OAlogNet,
                            "new bterm %s in %s %s index=%d.",
                            (const char*)tname,
                            Tstring(nh_sdesc->cellname()),
                            symbolic ? "symb" : "schem", pb->index());
                    }
                    if (symbolic) {
                        // There can be arbitrarily many pins per node in the
                        // symbolic rep.  For each additional pin, we add a
                        // point to the node property point list.  The symbol
                        // table keeps track of nodes we have seen before.

                        // Not all terminals in the schematic may be
                        // included in the symbol, in which case they
                        // should be invisible in the symbol.  We
                        // turn off symbol visibility of all
                        // terminals in the schematic, then turn back
                        // on those listed in the symbol.

                        if (pincnt == 0) {
                            pb->set_pos(0, x, y);
                            pb->unset_flag(TE_SYINVIS);
                        }
                        else {
                            unsigned int sz = pb->size_pos();
                            unsigned int ix = 0;
                            Point *pts = new Point[sz];
                            for ( ; ix < sz; ix++) {
                                if (!pb->get_pos(ix, &pts[ix].x, &pts[ix].y))
                                    break;
                            }
                            pb->alloc_pos(ix + 1);
                            sz = ix;  // sanity
                            for (ix = 0; ix < sz; ix++)
                                pb->set_pos(ix, pts[ix].x, pts[ix].y);
                            pb->set_pos(ix, x, y);
                            delete [] pts;
                        }
                        OAerrLog.add_log(OAlogNet,
                            "added symb pin location to %s: pincnt=%d"
                            " x=%d y=%d.", (const char*)tname, pincnt, x, y);
                    }
                    else {
                        if (pincnt == 0) {
                            pb->set_schem_pos(x, y);
                            pb->unset_flag(TE_SCINVIS);
                        }
                        else {
                            // To resolve a potentially split net, add
                            // a bus term device.

                            sLstr lstr;
                            pb->add_label_text(&lstr);
                            add_terminal(x, y, lstr.string());
                        }
                        OAerrLog.add_log(OAlogNet,
                            "Added schem pin location to %s: pincnt=%d"
                            " x=%d y=%d.", (const char*)tname, pincnt, x, y);
                    }

                    unsigned int ind = index_base;
                    CDnetexGen ngen(pb);
                    CDnetName nm;
                    int n;
                    while (ngen.next(&nm, &n)) {
                        CDnetName tnm = CDnetex::mk_name(Tstring(nm), n);
                        implement_bit(port_tab, term, Tstring(tnm),
                            symbolic, ind, pincnt, x, y);
                        ind++;
                    }
                }
                else {
                    implement_bit(port_tab, term, tname, symbolic, index_base,
                        pincnt, x, y);
                }
                pincnt++;
            }
            nterm++;
            if (!net->isGlobal() || key == P_NAME_TERM)
                npin += nbits;
        }

        // For subcircuit schematics, add node name mapping.
        //
        if (!symbolic && !nh_sdesc->isDevice()) {

            sLstr lstr;
            lstr.add("1 ");
            int cnt = 0;
            oaIter<oaNet> iter(nh_block->getNets());
            while (oaNet *net = iter.getNext()) {

                // Non-scalar nets aren't currently supported.
                if (net->getType() == oacBusNetType && net->isGlobal()) {
                    oaString nname;
                    net->getName(oaNativeNS(), nname);
                    OAerrLog.add_err(IFLOG_WARN,
                        "net %s is a global vector net, "
                        "globalness ignored.", (const char*)nname);
                }

                // This will recognize net name labels, both vector
                // and scalar.
                setup_wire_labels(net);

                if (net->getType() == oacBusNetBitType ||
                        net->getType() == oacScalarNetType) {

                    oaString net_name;
                    net->getName(oaNativeNS(), net_name);
                    if (net->isGlobal()) {
                        // Save the global name.
                        ScedIf()->registerGlobalNetName(net_name);
                    }
                    else {
                        // Non-global nets with terminals are already
                        // named by the terminal.

                        oaIter<oaTerm> te_iter(net->getTerms());
                        if (te_iter.getNext())
                            continue;
                    }

                    // Apply a net name record.
                    oaIter<oaShape> shape_iter(net->getShapes());
                    while (oaShape *shape = shape_iter.getNext()) {
                        if (shape->getType() == oacLineType) {
                            oaLine *line = (oaLine*)shape;
                            oaPointArray points;
                            line->getPoints(points);
                            int x = points.get(0).x();
                            int y = points.get(0).y();
                            if (nh_scale != 1) {
                                x *= nh_scale;
                                y *= nh_scale;
                            }
                            if (lstr.string())
                                lstr.add_c(' ');
                            lstr.add(net_name);
                            lstr.add_c(' ');
                            lstr.add_i(x);
                            lstr.add_c(' ');
                            lstr.add_i(y);
                            cnt++;
                            break;
                        }
                    }
                }
            }
            if (cnt)
                nh_sdesc->prptyAdd(P_NODMAP, lstr.string());
        }
    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    return (ret);
}


// A debugging/exploring tool, print things found in the oaBlock.
//
bool
cOAnetHandler::debugPrint()
{
    bool ret = true;
    try {
        oaDesign *design = nh_block->getDesign();
        oaString libname, cellname, viewname;
        design->getLibName(oaNativeNS(), libname);
        design->getCellName(oaNativeNS(), cellname);
        design->getViewName(oaNativeNS(), viewname);
        printf("%s %s %s\n", (const char*)libname, (const char*)cellname,
            (const char*)viewname);

        {
            int cnt = 0;
            oaIter<oaAssignment> iter(nh_block->getAssignments());
            while (oaAssignment *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Assignments %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaBlockage> iter(nh_block->getBlockages());
            while (oaBlockage *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Blockages %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaAreaBoundary> iter(nh_block->getBoundaries());
            while (oaAreaBoundary *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  AreaBoundaries %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaBusNetDef> iter(nh_block->getBusNetDefs());
            while (oaBusNetDef *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  BusNetDefs %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaBusTermDef> iter(nh_block->getBusTermDefs());
            while (oaBusTermDef *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  BusTermDefs %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaConnectDef> iter(nh_block->getConnectDefs());
            while (oaConnectDef *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  ConnectDefs %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaCluster> iter(nh_block->getClusters());
            while (oaCluster *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Clusters %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaFigGroup> iter(nh_block->getFigGroups());
            while (oaFigGroup *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  FigGroups %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaGCellPattern> iter(nh_block->getGCellPatterns());
            while (oaGCellPattern *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  GCellPatterns %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaGuide> iter(nh_block->getGuides());
            while (oaGuide *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Guides %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaInstHeader> iter(nh_block->getInstHeaders());
            while (oaInstHeader *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  InstHeaders %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaInst> iter(nh_block->getInsts());
            while (oaInst *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Insts %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaInstTerm> iter(nh_block->getInstTerms());
            while (oaInstTerm *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  InstTerms %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaLPPHeader> iter(nh_block->getLPPHeaders());
            while (oaLPPHeader *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  LPPHeaders %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaLayerHeader> iter(nh_block->getLayerHeaders());
            while (oaLayerHeader *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  LayerHeaders %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaMarker> iter(nh_block->getMarkers());
            while (oaMarker *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Markers %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaPin> iter(nh_block->getPins());
            while (oaPin *elt = iter.getNext()) {
                oaString pin_name;
                elt->getName(pin_name);

                printf("   %s %d %d %d\n", (const char*)pin_name,
                    (int)elt->getAccessDir(), (int)elt->getPinType(),
                    (int)elt->getPlacementStatus());
                oaIter<oaPinFig> sh_iter(elt->getFigs());

                while (oaPinFig *shape = sh_iter.getNext()) {
                    oaString shn = shape->getType().getName();
                    if (shape->getType() == oacRectType) {
                        oaBox bb;
                        ((oaRect*)shape)->getBBox(bb);
                        printf("    %s %d,%d %d,%d\n", (const char*)shn,
                            bb.left(), bb.bottom(), bb.right(), bb.top());
                    }
                    else
                        printf("    %s\n", (const char*)shn);
                }

                cnt++;
            }
            if (cnt > 0)
                printf("  Pins %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaNet> iter(nh_block->getNets());
            while (oaNet *elt = iter.getNext()) {
                oaString net_name;
                elt->getName(oaNativeNS(), net_name);

                int scnt = 0;
                oaIter<oaShape> sh_iter(elt->getShapes());
                while (oaShape *shape = sh_iter.getNext()) {
                    (void)shape;
                    scnt++;
                }
                printf("   %s %d %d %d %d\n", (const char*)net_name,
                    (int)elt->getSigType(), (int)elt->getPriority(),
                    (int)elt->getSource(), scnt);

                cnt++;
            }
            if (cnt > 0)
                printf("  Nets %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaRoute> iter(nh_block->getRoutes());
            while (oaRoute *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Routes %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaRowHeader> iter(nh_block->getRowHeaders());
            while (oaRowHeader *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  RowHeaders %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaRow> iter(nh_block->getRows());
            while (oaRow *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Rows %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaScanChain> iter(nh_block->getScanChains());
            while (oaScanChain *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  ScanChains %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaShape> iter(nh_block->getShapes());
            while (oaShape *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Shapes %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaSteiner> iter(nh_block->getSteiners());
            while (oaSteiner *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Steiners %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaTerm> iter(nh_block->getTerms());
            while (oaTerm *elt = iter.getNext()) {
                oaString term_name;
                elt->getName(oaNativeNS(), term_name);
                printf("   %s %d %d %d\n", (const char*)term_name,
                    (int)elt->getTermType(), (int)elt->getPosition(),
                    (int)elt->getNumBits());
                cnt++;
            }
            if (cnt > 0)
                printf("  Terms %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaTrackPattern> iter(nh_block->getTrackPatterns());
            while (oaTrackPattern *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  TrackPatterns %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaVectorInstDef> iter(nh_block->getVectorInstDefs());
            while (oaVectorInstDef *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  VectorInstDefs %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaVia> iter(nh_block->getVias());
            while (oaVia *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  Vias %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaViaHeader> iter(nh_block->getViaHeaders());
            while (oaViaHeader *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  ViaHeaders %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaGroupMember> iter(nh_block->getGroupLeaders());
            while (oaGroupMember *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  GroupLeaders %d\n", cnt);
        }

        {
            int cnt = 0;
            oaIter<oaGroupMember> iter(nh_block->getGroupMems());
            while (oaGroupMember *elt = iter.getNext()) {
                (void)elt;
                cnt++;
            }
            if (cnt > 0)
                printf("  GroupMems %d\n", cnt);
        }

    }
    catch (oaCompatibilityError &ex) {
        cOA::handleFBCError(ex);
        ret = false;
    }
    catch (oaException &excp) {
        Errs()->add_error((const char*)excp.getMsg());
        ret = false;
    }
    return (ret);
}


// This is not good.  In Xic each terminal connects at a specific
// coordinate, not by a shape.  Try to obtain a coordinate from the
// shape, return true if a coordinate is found.
//
bool
cOAnetHandler::find_pin_coords(const oaPin *pin, int key, bool symbolic,
    int *px, int *py)
{
    if (px)
        *px = 0;
    if (py)
        *py = 0;

    // Consider only one implementing object, ignore if none.
    oaIter<oaPinFig> sh_iter(pin->getFigs());
    oaPinFig *shape = sh_iter.getNext();
    if (!shape) {
        // This probably can't happen?
        if (OAerrLog.debug_net()) {
            oaString tname;
            oaTerm *term = pin->getTerm();
            if (term)
                term->getName(oaNativeNS(), tname);
            OAerrLog.add_log(OAlogNet, "terminal %s has no shape!",
                (const char*)tname);
        }
        return (false);
    }
    oaType sh_type = shape->getType();

    oaBox bb;
    if (sh_type == oacRectType) {
        oaRect *rect = (oaRect*)shape;
        rect->getBBox(bb);
    }
    else if (sh_type == oacPolygonType) {
        oaPolygon *polygon = (oaPolygon*)shape;
        polygon->getBBox(bb);
    }
    else if (sh_type == oacScalarInstType) {
        oaScalarInst *inst = (oaScalarInst*)shape;

        // Find the bounding box of rects/polys/wires.  Specifically,
        // we want to eliminate text labels.

        oaScalarName libName, cellName, viewName;
        inst->getLibName(libName);
        inst->getCellName(cellName);
        inst->getViewName(viewName);
        int nshapes = 0;
        oaDesign *des = oaDesign::open(libName, cellName, viewName, 'r');
        if (des) {
            oaBlock *blk = des->getTopBlock();
            if (blk) {
                oaIter<oaShape> shape_iter(blk->getShapes());
                while (oaShape *shape = shape_iter.getNext()) {
                    switch (shape->getType()) {
                    case oacRectType:
                    case oacPolygonType:
                    case oacPathType:
                    case oacPathSegType:
                        break;
                    default:
                        continue;
                    }
                    oaBox tbb;
                    shape->getBBox(tbb);
                    if (nshapes == 0)
                        bb = tbb;
                    else
                        bb.merge(tbb);
                    nshapes++;
                }
                oaTransform tx;
                inst->getTransform(tx);
                bb.transform(tx);
            }
            des->close();
        }
        if (!nshapes)
            inst->getBBox(bb);
    }
    else {
        if (OAerrLog.debug_net()) {
            oaString tname;
            oaTerm *term = pin->getTerm();
            if (term)
                term->getName(oaNativeNS(), tname);
            oaString shp = sh_type.getName();
            OAerrLog.add_log(OAlogNet, "terminal %s, unhandled shape %s.",
                (const char*)tname, (const char*)shp);
        }
        return (false);
    }

    if (nh_scale != 1) {
        bb.set(nh_scale*bb.left(), nh_scale*bb.bottom(),
            nh_scale*bb.right(), nh_scale*bb.top());
    }
    bool found_connection = false;
    int x = 0;
    int y = 0;
    if (!symbolic && key != P_NAME_TERM) {
        // Schematic subcircuit view, look for underlying
        // connection point.

        BBox AOI(bb.left(), bb.bottom(), bb.right(), bb.top());
        AOI.bloat(100);
        if (check_vertex(&AOI, &x, &y))
            found_connection = true;
        else if (OAerrLog.debug_net()) {
            oaString tname;
            oaTerm *term = pin->getTerm();
            if (term)
                term->getName(oaNativeNS(), tname);
            OAerrLog.add_log(OAlogNet,
                "terminal %s, no connection found! (%d,%d %d,%d)",
                (const char*)tname, AOI.left, AOI.bottom, AOI.right, AOI.top);
        }
    }
    if (!found_connection) {
        // Take the terminal location as the box center.
        x = (bb.left() + bb.right())/2;
        if (x % CDelecResolution != 0) {
            int xx = abs(x) + CDelecResolution/2;
            xx /= CDelecResolution;
            xx *= CDelecResolution;
            x = (x >= 0 ? xx : -xx);
        }
        y = (bb.bottom() + bb.top())/2;
        if (y % CDelecResolution != 0) {
            int yy = abs(y) + CDelecResolution/2;
            yy /= CDelecResolution;
            yy *= CDelecResolution;
            y = (y >= 0 ? yy : -yy);
        }
    }
    *px = x;
    *py = y;
    return (true);
}


bool
cOAnetHandler::implement_bit(const sOAportTab &port_tab, const oaTerm *term,
    const oaString &tname, bool symbolic, int ind, int pincnt, int x, int y)
{
    CDnetName bitbase;
    int bitnum;
    if (!CDnetex::parse_bit(tname, &bitbase, &bitnum)) {
        OAerrLog.add_err(IFLOG_WARN, "parse_error for %s, %s.",
            (const char*)tname, Errs()->get_error());
        return (false);
    }

    CDp_snode *ps = (CDp_snode*)nh_sdesc->prpty(P_NODE);
    for ( ; ps; ps = ps->next()) {
        CDnetName nm;
        int ix;
        if (!CDnetex::parse_bit(Tstring(ps->term_name()), &nm, &ix))
            continue;
        if (nm == bitbase && ix == bitnum)
            break;
    }
    if (!ps) {
        ps = new CDp_snode;
        ps->set_enode(-1);
        ps->set_term_name(tname);
        // Add a physical terminal.
        CDcbin cbin(nh_sdesc);
        CDsterm *tt;
        if (cbin.phys())
            tt = cbin.phys()->findPinTerm(ps->term_name(), true);
        else {
            tt = new CDsterm(0, ps->term_name());
            ps->set_flag(TE_OWNTERM);
        }
        ps->set_terminal(tt);
        tt->set_uninit(true);
        tt->set_node_prpty(ps);
        ps->set_next_prp(nh_sdesc->prptyList());
        nh_sdesc->setPrptyList(ps);

        unsigned int f = 0;
        ps->set_index(ind);
        if (term->getType() == oacBusTermType ||
                term->getType() == oacBundleTermType) {

            // Use the CDF port number if found for the index.  These are
            // probably not in the expected order.  Not to worry, mapping
            // will be automatic.

            unsigned int p;
            if (port_tab.find(tname, 0, &p))
                ps->set_index(p);

            // If the term is a vector, then the node properties are
            // the term bits.  Here we will make these invisible, and
            // put them all in one location which requires setting to
            // BYNAME flag so they don't short together in the
            // instance placement.  This will be undone if the bits
            // are actually placed in the schematic, which is likely
            // if the bit has CDF data.

            f |= (TE_BYNAME | TE_SCINVIS | TE_SYINVIS);
        }
        ps->set_flag(f);
        OAerrLog.add_log(OAlogNet, "new node %s in %s %s flg=Ox%x indx=%d.",
            (const char*)tname, Tstring(nh_sdesc->cellname()),
            symbolic ? "symb" : "schem", f, ps->index());
    }

    if (symbolic) {
        // There can be arbitrarily many pins per node in the
        // symbolic rep.  For each additional pin, we add a
        // point to the node property point list.  The symbol
        // table keeps track of nodes we have seen before.

        // Not all terminals in the schematic may be
        // included in the symbol, in which case they
        // should be invisible in the symbol.  We
        // turn off symbol visibility of all
        // terminals in the schematic, then turn back
        // on those listed in the symbol.

        if (pincnt == 0) {
            ps->set_pos(0, x, y);
            if (term->getType() == oacBusTermBitType) {
                // If a vector term bit is specifically listed here,
                // then is will be placed, so make it visible and
                // connectable.

                ps->unset_flag(TE_SYINVIS);
                ps->unset_flag(TE_BYNAME);
            }
        }
        else {
            unsigned int sz = ps->size_pos();
            unsigned int ix = 0;
            Point *pts = new Point[sz];
            for ( ; ix < sz; ix++) {
                if (!ps->get_pos(ix, &pts[ix].x, &pts[ix].y))
                    break;
            }
            ps->alloc_pos(ix + 1);
            sz = ix;  // sanity
            for (ix = 0; ix < sz; ix++)
                ps->set_pos(ix, pts[ix].x, pts[ix].y);
            ps->set_pos(ix, x, y);
            delete [] pts;
        }
        OAerrLog.add_log(OAlogNet,
            "added symb pin location to %s: pincnt=%d x=%d y=%d.",
            (const char*)tname, pincnt, x, y);
    }
    else {
        if (pincnt == 0) {
            ps->set_schem_pos(x, y);
            if (term->getType() == oacBusTermBitType) {
                // If a vector term bit is specifically listed here,
                // then is will be placed, so make it visible and
                // connectable.

                ps->unset_flag(TE_SCINVIS);
                ps->unset_flag(TE_BYNAME);
            }
        }
        else {
            // To resolve a potentially split net, add
            // a term device.

            add_terminal(x, y, tname);
        }
        OAerrLog.add_log(OAlogNet,
            "added schem pin location to %s: pincnt=%d x=%d y=%d.",
            (const char*)tname, pincnt, x, y);
    }
    return (true);
}


namespace {
    // Strip off trailing indices and compare base name, return true
    // if the same, case insensitive.
    //
    bool port_cmp(const char *s1, const char *s2)
    {
        for (;;) {
            int c1 = isupper(*s1) ? tolower(*s1) : *s1;
            int c2 = isupper(*s2) ? tolower(*s2) : *s2;
            if (c1 == '<' || c1 == '[' || c1 == '{')
                c1 = 0;
            if (c2 == '<' || c2 == '[' || c2 == '{')
                c2 = 0;
            if (c1 != c2)
                return (false);
            if (!c1)
                break;
            s1++;
            s2++;
        }
        return (true);
    }
}


// Determine and return the index number of the terminal whose name is
// given in tname.  We look at the portOrder property first, then the
// termOrder in the CDFF If no resolution, we apply a hard coded
// default for some devices, and if all else fails defnum is returned.
//
int
cOAnetHandler::port_order(const cOAelecInfo *cdf, int defnum,
    const oaString &tname, int key)
{
    if (key == P_NAME_TERM)
        return (0);
    if (cdf) {
        const char *const *terms = cdf->ports();
        if (!terms)
            terms = cdf->terms();
        if (terms) {
            int tnum = 0;
            for ( ; *terms; terms++) {
                if (port_cmp(tname, *terms))
                    return (tnum);
                tnum++;
            }
        }

        OAerrLog.add_log(OAlogNet, "no ordering for %s %c %s.",
            (const char*)tname, key, cdf->name());
        const char *pfx = cdf->prefix();
        if (pfx) {
            int tnum = sOAportTab::def_order(tname, *pfx);
            if (tnum >= 0)
                return (tnum);
        }
    }
    else {
        OAerrLog.add_log(OAlogNet, "no ordering/CDF for %s %c.",
            (const char*)tname, key);
    }
    int tnum = sOAportTab::def_order(tname, key);
    if (tnum >= 0)
        return (tnum);
    return (defnum);
}


// Look for a connection point touching AOI, if found return true
// and set px/py.  This is a bit tricky since BBs probably are mot
// valid yet, nor have instance properties been applied. 
// Instances may not be in the spatial database yet.
//
bool
cOAnetHandler::check_vertex(const BBox *AOI, int *px, int *py)
{
    CDsLgen gen(nh_sdesc);
    CDl *ld;
    CDg gdesc;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(nh_sdesc, ld, AOI);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE)
                continue;
            if (!wd->is_normal())
                continue;
            const Point *pts = wd->points();
            int num = wd->numpts();
            for (int i = 0; i < num; i++) {
                if (AOI->intersect(&pts[i], true)) {
                    *px = pts[i].x;
                    *py = pts[i].y;
                    return (true);
                }
            }
        }
    }
    cTfmStack stk;
    CDm_gen mgen(nh_sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;

        // Look for connections to devices and subcircuits.
        if (!isDevOrSubc(msdesc->elecCellType()))
            continue;

        CDp_snode *pn0 = (CDp_snode*)msdesc->prpty(P_NODE);
        CDp_bsnode *pb0 = (CDp_bsnode*)msdesc->prpty(P_BNODE);

        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            stk.TPush();
            stk.TApplyTransform(cdesc);

            bool symbolic = msdesc->symbolicRep(cdesc);
            for (CDp_snode *pn = pn0; pn; pn = pn->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (symbolic) {
                        if (!pn->get_pos(ix, &x, &y))
                            break;
                    }
                    else {
                        if (ix)
                            break;
                        pn->get_schem_pos(&x, &y);
                    }
                    stk.TPoint(&x, &y);
                    if (AOI->intersect(x, y, true)) {
                        *px = x;
                        *py = y;
                        return (true);
                    }
                }
            }
            for (CDp_bsnode *pb = pb0; pb; pb = pb->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (symbolic) {
                        if (!pb->get_pos(ix, &x, &y))
                            break;
                    }
                    else {
                        if (ix)
                            break;
                        pb->get_schem_pos(&x, &y);
                    }
                    stk.TPoint(&x, &y);
                    if (AOI->intersect(x, y, true)) {
                        *px = x;
                        *py = y;
                        return (true);
                    }
                }
            }
            stk.TPop();
        }
    }
    return (false);
}


// Look through the shapes associated with the net.  Attempt to link
// text labels to wires in the net.  In Xic, a wire with an associated
// label will supply a name to the containing net, much like a
// terminal device.
//
void
cOAnetHandler::setup_wire_labels(const oaNet *net)
{
    bool ret = true;
    oaIter<oaShape> shape_iter(net->getShapes());
    while (oaShape *shape = shape_iter.getNext()) {
        if (shape->getType() != oacTextType)
            continue;
        oaText *text = (oaText*)shape;
        if (!setup_wire_label(text))
            ret = false;
    }
    if (!ret) {
        oaString nname;
        net->getName(oaNativeNS(), nname);

        sLstr lstr;
        oaIter<oaShape> xshape_iter(net->getShapes());
        while (oaShape *shape = xshape_iter.getNext()) {
            if (shape->getType() != oacTextType)
                continue;
            oaText *text = (oaText*)shape;
            oaString tt;
            text->getText(tt);
            lstr.add((const char*)tt);
            break;
        }
        if (!lstr.string())
            lstr.add("none");

        OAerrLog.add_err(IFLOG_WARN, "net %s, label %s, %s.",
            (const char*)nname, lstr.string(), Errs()->get_error());
    }
}


namespace {
    // The net contains a text label.  How is this "connected" to a
    // wire?  Some hackery:  The oaText and oaLine/oaPath participate
    // in a group (oaGroup) with group name
    // "__CDBA_PARENTCHILD_ONLY_GROUP".  The (exactly) two elements of
    // the group are the text and associated line/fat line.
    //
    oaShape *findAssociatedObj(const oaText *text)
    {
        if (!text || !text->inGroup())
            return (0);
        oaIter<oaGroup> grp_iter(text->getGroupsOwnedBy());
        while (oaGroup *grp = grp_iter.getNext()) {
            oaString name;
            grp->getName(name);
            if (name != "__CDBA_PARENTCHILD_ONLY_GROUP")
                continue;
            oaIter<oaGroupMember> mbr_iter(grp->getMembers());
            while (oaGroupMember *m = mbr_iter.getNext()) {
                oaType tp = m->getObject()->getType();
                if (tp == oacLineType || tp == oacPathType)
                    return ((oaShape*)m->getObject());
            }
        }
        return (0);
    }
}


bool
cOAnetHandler::setup_wire_label(const oaText *text)
{
    if (!text) {
        Errs()->add_error("wire label setup: null oaText pointer.");
        return (false);
    }

    oaShape *shape = findAssociatedObj(text);
    if (!shape) {
        Errs()->add_error("wire label setup: null oaShape pointer.");
        return (false);
    }
    // The shape is eather oaLine or oaPath, the latter for "fat"
    // wires.

    // Find the corresponding Xic label, by position and LPP.  Assume
    // it is unique (should probably check this).
    CDla *label = 0;
    oaPoint org;
    text->getOrigin(org);
    if (nh_scale != 1)
        org.set(nh_scale * org.x(), nh_scale * org.y());
    CDl *ld = CDldb()->findLayer(text->getLayerNum(), text->getPurposeNum());
    BBox BB(org.x(), org.y(), org.x(), org.y());
    CDg gdesc;
    gdesc.init_gen(nh_sdesc, ld, &BB);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (!odesc->is_normal())
            continue;
        if (odesc->type() != CDLABEL)
            continue;
        CDla *la = (CDla*)odesc;
        if (la->xpos() == org.x() && la->ypos() == org.y()) {
            label = la;
            break;
        }
    }
    if (!label) {
        Errs()->add_error("wire label setup: failed to find native label.");
        return (false);
    }

    // Now find the wire that corresponds to the line/path.
    CDw *wire = 0;
    oaPointArray points;
    if (shape->getType() == oacLineType)
        ((oaLine*)shape)->getPoints(points);
    else if (shape->getType() == oacPathType)
        ((oaPath*)shape)->getPoints(points);
    else {
        Errs()->add_error("wire label setup: unhandled shape %s.",
            (const char*)shape->getType().getName());
        return (false);
    }

    int x = points[0].x() * nh_scale;
    int y = points[0].y() * nh_scale;
    ld = CDldb()->findLayer(shape->getLayerNum(), shape->getPurposeNum());
    BB = BBox(x, y, x, y);
    gdesc.init_gen(nh_sdesc, ld, &BB);
    while ((odesc = gdesc.next()) != 0) {
        if (!odesc->is_normal())
            continue;
        if (odesc->type() != CDWIRE)
            continue;
        CDw *w = (CDw*)odesc;
        if ((unsigned int)w->numpts() != points.getNumElements())
            continue;
        bool nogood = false;
        for (int i = 0; i < w->numpts(); i++) {
            if (w->points()[i].x != points[i].x() * nh_scale ||
                    w->points()[i].y != points[i].y() * nh_scale) {
                nogood = true;
                break;
            }
        }
        if (!nogood) {
            wire = w;
            break;
        }
    }
    if (!wire) {
        Errs()->add_error("wire label setup: failed to find native wire.");
        return (false);
    }
    return (wire->set_node_label(nh_sdesc, label, true));
}


#define TERM_DEV "txbox"

// Add an Xic terminal at x,y using the net name tname.  The objects
// are created directly without calling the undo system.
//
bool
cOAnetHandler::add_terminal(int x, int y, const char *tname)
{
    CDcbin cbin;
    if (!CDcdb()->findSymbol(TERM_DEV, &cbin) &&
            OIfailed(FIO()->FromNative(TERM_DEV, &cbin, 1.0)))
        return (false);

    CDs *msdesc = cbin.elec();
    if (!msdesc)
        return (false);
    CallDesc calldesc;
    calldesc.setName(cbin.cellname());
    calldesc.setCelldesc(msdesc);

    // Terminal 0 is the reference point.
    int xr = 0, yr = 0;
    CDp_snode *pn = (CDp_snode*)msdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {   
        if (pn->index() == 0) {
            if (nh_sdesc->symbolicRep(0))
                pn->get_pos(0, &xr, &yr);
            else
                pn->get_schem_pos(&xr, &yr);
            break;
        }
    }
    cTfmStack stk;
    stk.TPush();
    stk.TTranslate(x + xr, y + yr);
    CDtx tx;
    stk.TCurrent(&tx);
    stk.TPop();
    CDap ap;

    CDc *cdesc;
    if (nh_sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone, &cdesc) != OIok)
        return (false);

    // Check that the terminal device has a name property.
    CDp_name *pa = (CDp_name*)cdesc->prpty(P_NAME);
    if (!pa) {
        Errs()->add_error("Instance of %s has no name property.",
            Tstring(msdesc->cellname()));
        return (false);
    }

    Label label;
    label.label = new hyList(0, tname, HYcvAscii);
    if (!label.label)
        return (true);
    DSP()->DefaultLabelSize(label.label, Electrical, &label.width,
        &label.height);
    SCD()->labelPlacement(P_NAME, cdesc, &label);

    CDla *nlabel;
    if (nh_sdesc->makeLabel(SCD()->defaultLayer(pa), &label, &nlabel,
            false) != CDok)
        return (false);

    pa->bind(nlabel);
    if (!nlabel->link(nh_sdesc, cdesc, pa)) {
        return (false);
    }
    return (true);
}

