
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
#include "sced_nodemap.h"
#include "edit.h"
#include "undolist.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "cd_netname.h"
#include "cd_celldb.h"
#include "cd_lgen.h"
#include "fio.h"
#include "menu.h"
#include "ebtn_menu.h"


/*========================================================================*
 *
 * Functions for mapping node numbers to text strings
 *
 *========================================================================*/

void
cSced::setModified(cNodeMap *map)
{
    if (map)
        map->setDirty();
}


// Destroy the node info struct, zero pointer.
//
void
cSced::destroyNodes(CDs *sd)
{
    if (!sd || !sd->isElectrical())
        return;
    cNodeMap *m = sd->nodes();
    sd->setNodes(0);
    if (m) {
        delete m;
        sd->unsetConnected();
        CDs *tsd = CDcdb()->findCell(sd->cellname(), Physical);
        if (tsd)
            tsd->reflectBadAssoc();
    }
}


// Return the internal node number that matches the name, or -1 if not
// found.
//
int
cSced::findNode(const CDs *sd, const char *name)
{
    if (!sd || !sd->isElectrical())
        return (-1);
    cNodeMap *map = sd->nodes();
    if (!map)
        return (-1);
    return (map->findNode(name));
}


// Return the name for the node.
//
const char *
cSced::nodeName(const CDs *sd, int node, bool *glob)
{
    if (sd && sd->isElectrical() && sd->nodes()) {
        if (glob)
            *glob = sd->nodes()->isGlobal(node);
        return (sd->nodes()->map(node));
    }
    // senseless
    if (glob)
        *glob = false;
    char buf[64];
    mmItoA(buf, node);
    return (Tstring(CDnetex::name_tab_add(buf)));
}


// Update the P_NODMAP property.  Call this before the cell is saved to
// a file or duplicated.
//
void
cSced::updateNodes(const CDs *sd)
{
    if (sd && sd->isElectrical() && sd->nodes())
        sd->nodes()->updateProperty();
}


// Register the name of a global net.  We support scalar and vector
// global nets.  A scalar global net has been previously called a
// "global node".
//
void
cSced::registerGlobalNetName(const char *nn)
{
    if (!nn || !*nn)
        return;
    if (!sc_global_tab) {
        // Case sensitivity must match name string table.
        sc_global_tab = new SymTab(false, false);
        sc_global_tab->set_case_insens(CDnetex::name_tab_case_insens());
    }
    if (SymTab::get(sc_global_tab, nn) != ST_NIL)
        return;
    CDnetName nm = CDnetex::name_tab_add(nn);
    sc_global_tab->add(Tstring(nm), 0, false);
}


// Return true if the argument names a global net.
//
bool
cSced::isGlobalNetName(const char *nn)
{
    if (!nn || !*nn)
        return (false);

    // Names that end with '!' are always global.  These might not be
    // in the table.
    if (isalnum(*nn) && *(nn + strlen(nn) - 1) == '!') {
        registerGlobalNetName(nn);
        return (true);
    }

    if (!sc_global_tab)
        return (false);
    return (SymTab::get(sc_global_tab, nn) != ST_NIL);
}


// Return a hash table containing all of the global net names found in
// the sdesc hierarchy.  The name is indexed as an unsigned long
// CDnetex string table entry, with null data.  Caller should delete
// the returned table.
//
SymTab *
cSced::tabGlobals(CDs *sdesc)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    SymTab *tab = new SymTab(false, false);
    CDgenHierDn_s gen(sdesc);
    bool err;
    CDs *sd;
    while ((sd = gen.next(&err)) != 0) {
        cNodeMap *map = sd->nodes();
        if (map)
            map->tabAddGlobal(tab);
    }
    return (tab);
}
// End of cSced functions.


sNodeName::~sNodeName()
{
    delete nn_hent;
}
// End of sNodeName functions.


cNodeMap::cNodeMap(CDs *sdesc)   // electrical arg
{
    nm_nmap = 0;
    nm_fmap = 0;
    nm_setnames = 0;
    nm_celldesc = sdesc;
    nm_netname_tab = 0;
    nm_size = 0;
    nm_connect_size = 0;
    nm_dirty = true;

    extract_setnames();
}


int
cNodeMap::findNode(const char *n)
{
    if (!n)
        return (-1);
    return (findNode(CDnetex::name_tab_add(n)));
}


// Hunt through the map to obtain the node index corresponding to the
// given name.  This will update the map if necessary.
//
int
cNodeMap::findNode(CDnetName name)
{
    if (!name)
        return (-1);
    refresh();  // makes this function non-const

    if (nm_netname_tab) {
        long n = (long)SymTab::get(nm_netname_tab, (unsigned long)name);
        if (n >= 0)
            return (n);
    }

    char buf[64];
    for (int i = 0; i < nm_size; i++) {
        CDnetName n = nm_nmap[i];
        if (!n) {
            sprintf(buf, "%d", i);
            n = CDnetex::name_tab_add(buf);
            nm_nmap[i] = n;
        }
        if (n == name)
            return (i);
    }

    // No name match.  If the name is an integer in range, return the
    // node.
    bool isnum = true;
    for (const char *s = Tstring(name); *s; s++) {
        if (!isdigit(*s)) {
            isnum = false;
            break;
        }
    }

    int d;
    if (isnum && sscanf(Tstring(name), "%d", &d) == 1 && d >= 0 &&
            d < nm_size)
        return (d);
    return (-1);
}


// Return the required size of the map.
//
int
cNodeMap::countNodes()
{
    if (!nm_dirty)
        return (nm_size);
    SCD()->connect(nm_celldesc);
    if (!nm_connect_size)
        nm_connect_size = 1;
    return (nm_connect_size);
}


// Callback from the connection operation.  The nmtab tags are net
// names as unsigned long CDnetName, with data being the node number.
// Note that we take ownership here.
//
void
cNodeMap::setupNetNames(int csize, SymTab *nmtab)
{
    nm_connect_size = csize;
    delete nm_netname_tab;
    nm_netname_tab = nmtab;
}


// Add a new entry to the setnames list, and clear any bogus entries,
// map is left dirty if entry added or changed.
//
bool
cNodeMap::newEntry(const char *nm, int node)
{
    if (!nm)
        return (false);
    CDnetName name = CDnetex::name_tab_add(nm);
    bool already_there = false;
    bool renamed = false;
    sNodeName *sp = 0, *snext;
    for (sNodeName *sn = nm_setnames; sn; sn = snext) {
        snext = sn->next();
        int n = sn->hent()->nodenum();
        if (n < 0) {
            if (sp)
                sp->set_next(snext);
            else
                nm_setnames = snext;
            delete sn;
            continue;
        }
        if (n == node) {
            // Node already named, just change to new name.
            if (sn->name() != name) {
                sn->set_name(name);
                renamed = true;
            }
            already_there = true;
        }
        sp = sn;
    }
    if (already_there) {
        if (renamed) {
            nm_dirty = true;
            nm_celldesc->incModified();
            nm_celldesc->unsetConnected();
        }
        return (true);
    }

    // Find a device terminal for this node (there must be one) and
    // create a new link.
    CDm_gen mgen(nm_celldesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;
        // Skip terminal devices.
        if (msdesc->elecCellType() == CDelecTerm)
            continue;
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (!cdesc->is_normal())
                continue;

            CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pc; pc = pc->next()) {
                if (pc->enode() == node) {

                    // There can be more than one terminal location, use
                    // the first one.
                    int x, y;
                    if (!pc->get_pos(0, &x, &y))
                        continue;

                    hyEnt *h = new hyEnt(nm_celldesc, x, y, cdesc, HYrefNode,
                        HYorNone);
                    h->add();

                    sNodeName *se = nm_setnames;
                    if (se) {
                        while (se->next())
                            se = se->next();
                        se->set_next(new sNodeName(name, h, 0));
                    }
                    else
                        nm_setnames = new sNodeName(name, h, 0);

                    nm_dirty = true;
                    nm_celldesc->incModified();
                    nm_celldesc->unsetConnected();
                    return (true);
                }
            }
        }
    }

    // Bad node number, no matching terminal.
    Errs()->add_error(
        "net for node %d contains no device or subcircuit connection.", node);
    return (false);
}


// Remove the setnames entry for node, and any bogus entries.  If a
// valid entry is removed, the dirty flag is set.
//
void
cNodeMap::delEntry(int node)
{
    int cnt = 0;
    sNodeName *sp = 0, *snext;
    for (sNodeName *sn = nm_setnames; sn; sn = snext) {
        snext = sn->next();
        int n = sn->hent()->nodenum();
        if (n == node || n < 0) {
            if (sp)
                sp->set_next(snext);
            else
                nm_setnames = snext;
            delete sn;
            if (n >= 0) {
                if (cnt == 0) {
                    nm_dirty = true;
                    nm_celldesc->incModified();
                    nm_celldesc->unsetConnected();
                }
                cnt++;
            }
            continue;
        }
        sp = sn;
    }
}


// Given the node number, return the mapped name in buf if possible,
// else the default name.  The map is used only if active and
// up-to-date.
//
const char *
cNodeMap::map(int i) const
{
    if (nm_dirty || i < 0 || i >= nm_size || !nm_nmap[i]) {
        char buf[64];
        mmItoA(buf, i);
        return (Tstring(CDnetex::name_tab_add(buf)));
    }
    return (Tstring(nm_nmap[i]));
}


CDnetName
cNodeMap::mapStab(int i) const
{
    if (nm_dirty || i < 0 || i >= nm_size || !nm_nmap[i])
        return (0);
    return (nm_nmap[i]);
}


// Given the node number, return the name if up-to-date.
//
const char *
cNodeMap::mapName(int i) const
{
    if (nm_dirty || i < 0 || i >= nm_size || !nm_nmap[i])
        return ("");
    return (Tstring(nm_nmap[i]));
}


// Return true if the node has a user-defined name.
//
bool
cNodeMap::isSet(int i) const
{
    if (nm_dirty || i < 0 || i >= nm_size || !(nm_fmap[i] & NM_SET))
        return (false);
    return (true);
}


// Return true if the node has global name.
//
bool
cNodeMap::isGlobal(int i) const
{
    if (nm_dirty || i < 0 || i >= nm_size)
        return (false);
    if (!nm_nmap[i]) {
        nm_fmap[i] &= ~NM_GLOB;
        return (false);
    }
    if (nm_fmap[i] & NM_GLOB)
        return (true);
    if (SCD()->isGlobalNetName(Tstring(nm_nmap[i]))) {
        nm_fmap[i] |= NM_GLOB;
        return (true);
    }
    return (false);
}


// Return the number of connections to global nets if count is true,
// otherwise return true on the first global.
//
int
cNodeMap::hasGlobal(bool count) const
{
    if (nm_dirty)
        return (0);
    int cnt = 0;
    for (int i = 0; i < nm_size; i++) {
        if (nm_fmap[i] & NM_GLOB) {
            if (!count)
                return (1);
            cnt++;
        }
    }
    return (cnt);
}


// Record each global name in the passed symbol table.
//
void
cNodeMap::tabAddGlobal(SymTab *tab) const
{
    if (nm_dirty || !tab)
        return;
    for (int i = 0; i < nm_size; i++) {
        if (nm_fmap[i] & NM_GLOB)
            tab->add((unsigned long)nm_nmap[i], 0, true);
    }
}


// Update/create the P_NODMAP property from the node map.
//
void
cNodeMap::updateProperty()
{
    refresh();

    CDp_nodmp *pn = (CDp_nodmp*)nm_celldesc->prpty(P_NODMAP);
    if (!nm_setnames) {
        if (pn) {
            nm_celldesc->prptyUnlink(pn);
            delete pn;
        }
        return;
    }

    int cnt = 0;
    for (sNodeName *sn = nm_setnames; sn; sn = sn->next(), cnt++) ;
    CDnmapRef *list = new CDnmapRef[cnt];
    cnt = 0;
    for (sNodeName *sn = nm_setnames; sn; sn = sn->next()) {
        if (sn->hent()->ref_type() == HYrefNode) {
            list[cnt].name = sn->name();
            list[cnt].x = sn->hent()->pos_x();
            list[cnt].y = sn->hent()->pos_y();
            cnt++;
        }
    }
    if (!pn) {
        nm_celldesc->prptyAdd(P_NODMAP, "0");
        pn = (CDp_nodmp*)nm_celldesc->prpty(P_NODMAP);
    }
    if (pn) {
        delete [] pn->maplist();
        pn->set_maplist(list);
        pn->set_mapsize(cnt);
    }
}


// Return a list providing the assigned names and a related coordinate
// location.  The caller should free the list when done with
// xyname_t::free.
//
xyname_t *
cNodeMap::getSetList() const
{
    xyname_t *n0 = 0, *ne = 0;
    for (sNodeName *sn = nm_setnames; sn; sn = sn->next()) {
        if (sn->hent()->ref_type() == HYrefNode) {
            xyname_t *n = new xyname_t(sn->name(),
                sn->hent()->pos_x(), sn->hent()->pos_y());
            if (!n0)
                n0 = ne = n;
            else {
                ne->set_next(n);
                ne = n;
            }
        }
    }
    return (n0);
}


// Build the setnames list from the nodemap property, which is
// cleared.
//
void
cNodeMap::extract_setnames()
{
    CDp_nodmp *pn = (CDp_nodmp*)nm_celldesc->prpty(P_NODMAP);
    if (pn) {
        sNodeName *se = nm_setnames;
        if (se) {
            while (se->next())
                se = se->next();
        }

        cTfmStack stk;
        for (int i = 0; i < pn->mapsize(); i++) {
            hyEnt *h = nm_celldesc->hyNode(&stk, pn->maplist()[i].x,
                pn->maplist()[i].y);
            if (h) {
                h->add();
                if (!nm_setnames)
                    se = nm_setnames =
                        new sNodeName(pn->maplist()[i].name, h, 0);
                else {
                    se->set_next(
                        new sNodeName(pn->maplist()[i].name, h, 0));
                    se = se->next();
                }
            }
        }
        pn->set_mapsize(0);
        delete [] pn->maplist();
        pn->set_maplist(0);
        nm_celldesc->unsetConnected();
    }
}


namespace {
    // Add the global node names found in DefaultNode properties in
    // the device.lib file.
    //
    void addGlobalNodeNames()
    {
        const stringlist *sl =
            FIO()->GetLibraryProperties(XM()->DeviceLibName());
        for ( ; sl; sl = sl->next) {
            char *s = sl->string;
            char *ntok = lstring::gettok(&s);
            if (!ntok)
                continue;
            if (!strcasecmp(ntok, LpDefaultNodeStr) ||
                    LpDefaultNodeVal == atoi(ntok)) {
                lstring::advtok(&s);  // skip name
                lstring::advtok(&s);  // skip num
                char *tok = lstring::gettok(&s);
                if (tok)
                    SCD()->registerGlobalNetName(tok);
                delete [] tok;
            }
            delete [] ntok;
        }
    }
}


// Set up the node name and flags arrays.
//
void
cNodeMap::setup()
{
    if (nm_size <= 0)
        return;
    static bool did_globs;
    if (!did_globs) {
        did_globs = true;
        addGlobalNodeNames();
    }
    memset(nm_nmap, 0, nm_size*sizeof(CDnetName));
    memset(nm_fmap, 0, nm_size*sizeof(unsigned char));

    if (nm_netname_tab) {
        // The table should contain all of the names that have been
        // used, with possible multiple mappings into the same node. 
        // This includes the setnames and formal terminal names.

        SymTabGen gen(nm_netname_tab);
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            CDnetName nm = (CDnetName)ent->stTag;
            int node = (long)ent->stData;
            if (node < 0 || node >= nm_size)
                continue;

            if (!nm_nmap[node]) {
                nm_nmap[node] = nm;
                continue;
            }

            // If a name is already set, keep the name that is
            // global, or comes first in strcmp() order.

            CDnetName oname = nm_nmap[node];
            bool oglb = SCD()->isGlobalNetName(Tstring(oname));
            bool nglb = SCD()->isGlobalNetName(Tstring(nm));
            if (nglb && !oglb)
                nm_nmap[node] = nm;
            else if ((oglb == nglb) && 
                    strcmp(Tstring(nm), Tstring(oname)) < 0)
                nm_nmap[node] = nm;
        }
    }
    /*** Dead code now.
    else {

        // Apply names found in bound wire labels.
        CDsLgen gen(nm_celldesc);
        CDl *ld;
        while ((ld = gen.next()) != 0) {
            if (!ld->isWireActive())
                continue;
            CDg gdesc;
            gdesc.init_gen(nm_celldesc, ld);
            CDo *pointer;
            while ((pointer = gdesc.next()) != 0) {
                if (!pointer->is_normal())
                    continue;
                if (pointer->type() != CDWIRE)
                    continue;
                CDp_node *pn = (CDp_node*)pointer->prpty(P_NODE);
                if (!pn || !pn->term_name_stab())
                    continue;
                if (pn->enode() <= 0 || pn->enode() >= nm_size)
                    continue;
                if (!nm_nmap[pn->enode()]) {
                    nm_nmap[pn->enode()] = pn->term_name_stab();
                    continue;
                }

                // If a name is already set, keep the name that is
                // global, or comes first in strcmp() order.

                CDnetName oname = nm_nmap[pn->enode()];
                bool oglb = SCD()->isGlobalNodeName(oname->string());
                CDnetName nname = pn->term_name_stab();
                bool nglb = SCD()->isGlobalNodeName(nname->string());

                if (nglb && !oglb)
                    nm_nmap[pn->enode()] = nname;
                else if ((oglb == nglb) && 
                        strcmp(nname->string(), oname->string()) < 0)
                    nm_nmap[pn->enode()] = nname;
            }
        }

        // Apply names from attached terminal devices.
        CDm_gen mgen(nm_celldesc, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            CDs *msdesc = mdesc->celldesc();
            if (!msdesc)
                continue;
            CDelecCellType tp = msdesc->elecCellType();
            if (tp == CDelecTerm) {
                // A terminal.
                CDc_gen cgen(mdesc);
                for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                    CDp_cnode *pn = (CDp_cnode*)c->prpty(P_NODE);
                    if (!pn || !pn->term_name_stab())
                        continue;
                    if (pn->enode() <= 0 || pn->enode() >= nm_size)
                        continue;
                    if (!nm_nmap[pn->enode()]) {
                        nm_nmap[pn->enode()] = pn->term_name_stab();
                        continue;
                    }

                    // If a name is already set, keep the name that is
                    // global, or comes first in strcmp() order.

                    CDnetName oname = nm_nmap[pn->enode()];
                    bool oglb = SCD()->isGlobalNodeName(oname->string());
                    CDnetName nname = pn->term_name_stab();
                    bool nglb = SCD()->isGlobalNodeName(nname->string());

                    if (nglb && !oglb)
                        nm_nmap[pn->enode()] = nname;
                    else if ((oglb == nglb) &&
                            strcmp(nname->string(), oname->string()) < 0)
                        nm_nmap[pn->enode()] = nname;
                }
            }
        }
    }
    ***/

    // Override with cell terminal names.
    CDp_snode *ps = (CDp_snode*)nm_celldesc->prpty(P_NODE);
    for ( ; ps; ps = ps->next()) {
        if (!ps->get_term_name())
            continue;
        if (ps->enode() <= 0 || ps->enode() >= nm_size)
            continue;
        if (!nm_nmap[ps->enode()]) {
            nm_nmap[ps->enode()] = ps->get_term_name();
            continue;
        }

        CDnetName oname = nm_nmap[ps->enode()];
        bool oglb = SCD()->isGlobalNetName(Tstring(oname));
        if (!oglb) {
            // Terminal name has priority if existing name isn't
            // global.

            nm_nmap[ps->enode()] = ps->get_term_name();
            continue;
        }

        CDnetName nname = ps->get_term_name();
        bool nglb = SCD()->isGlobalNetName(Tstring(nname));
        if (nglb) {
            // Both global.  This is probably a user error, but keep
            // the name that comes first in strcmp order.

            if (strcmp(Tstring(nname), Tstring(oname)) < 0)
                nm_nmap[ps->enode()] = nname;
        }
    }

    // Override with the setnames.
    for (sNodeName *sn = nm_setnames; sn; sn = sn->next()) {
        int node = sn->hent()->nodenum();
        if (node <= 0 || node >= nm_size)
            continue;
        if (!nm_nmap[node]) {
            nm_nmap[node] = sn->name();
            nm_fmap[node] |= NM_SET;
            continue;
        }

        CDnetName oname = nm_nmap[node];
        bool oglb = SCD()->isGlobalNetName(Tstring(oname));
        if (!oglb) {
            // Set-name has priority if existing name isn't
            // global.

            nm_nmap[node] = sn->name();
            nm_fmap[node] |= NM_SET;
            continue;
        }

        CDnetName nname = sn->name();
        bool nglb = SCD()->isGlobalNetName(Tstring(nname));
        if (nglb) {
            // Both global.  This is probably a user error, but keep
            // the name that comes first in strcmp order.

            if (strcmp(Tstring(nname), Tstring(oname)) < 0) {
                nm_nmap[node] = nname;
                nm_fmap[node] |= NM_SET;
            }
        }
    }
    for (int i = 1; i < nm_size; i++) {
        if (SCD()->isGlobalNetName(Tstring(nm_nmap[i])))
            nm_fmap[i] |= NM_GLOB;
    }

    // All done, we're clean now.
    nm_dirty = false;
}


// Rebuild the mapping if not current.  The hierarchy for naming is:
//   1. Global names.  These override everything, including user-
//      supplied names.
//   2. User-supplied node names.
//   3. Names from cell terminals.
//   4. Names from terminal devices or wire labels.
//   7. Internally-generated names.
//
// If a map entry is 0, the internal name is implied.
//
void
cNodeMap::refresh()
{
    if (!nm_dirty)
        return;
    int osz = nm_size;
    nm_size = countNodes();
    if (osz != nm_size) {
        delete [] nm_nmap;
        delete [] nm_fmap;
        nm_nmap = new CDnetName[nm_size];
        nm_fmap = new unsigned char[nm_size];
    }
    setup();
}

