
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
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
 $Id: cd_cell.cc,v 5.188 2016/05/31 06:23:09 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_terminal.h"
#include "cd_celldb.h"
#include "fio.h"
#include "fio_library.h"
#include <ctype.h>


FlagDef SdescFlags[] =
{
    { "BBVALID", CDs_BBVALID, false, "Bounding box is valid" },
    { "BBSUBNG", CDs_BBSUBNG, false, "Subcell has unknown bounding box" },
    { "ELECTR", CDs_ELECTR, false, "Cell contains electrical data" },
    { "SYMBOLIC", CDs_SYMBOLIC, false, "Symbolic representation"},
    { "CONNECT", CDs_CONNECT, false, "Connectivity info is current" },
    { "GPINV", CDs_GPINV, false, "Inverted ground plane current" },
    { "DSEXT", CDs_DSEXT, false, "Devices and subcircuits extracted" },
    { "DUALS", CDs_DUALS, false, "Physical/electrical duality established" },
    { "UNREAD", CDs_UNREAD, false, "Created to satisfy unsatisfied reference" },
    { "COMPRESSED", CDs_COMPRESSED, false, "Save hierarchy in compressed form" },
    { "SAVNTV", CDs_SAVNTV, false, "Save in native format before exit" },
    { "ALTERED", CDs_ALTERED, false, "Cell data were altered when read" },
    { "CHDREF", CDs_CHDREF, false, "Cell is a reference" },
    { "DEVICE", CDs_DEVICE, false, "Cell represents a device symbol" },
    { "LIBRARY", CDs_LIBRARY, true, "Cell is from a library" },
    { "IMMUTABLE", CDs_IMMUTABLE, true, "Cell is read-only" },
    { "OPAQUE", CDs_OPAQUE, true, "Cell content is ignored in extraction" },
    { "CONNECTOR", CDs_CONNECTOR, true, "Cell is a connector" },
    { "SPCONNECT", CDs_SPCONNECT, false, "SPICE connectivity info is curent" },
    { "USER0", CDs_USER, true, "User flag 0" },
    { "USER1", CDs_USER*2, true, "User flag 1" },
    { "PCELL", CDs_PCELL, false, "Cell is a PCell sub- or super-master" },
    { "PCSUPR", CDs_PCSUPR, false, "Cell is a PCell super-master" },
    { "PCOA", CDs_PCOA, false, "Cell is a PCell sub-master from OpenAccess" },
    { "PCKEEP", CDs_PCKEEP, false, "PCell sub-master read from file" },
    { "STDVIA", CDs_STDVIA, false, "Cell is a standard via sub-master" },
    { 0, 0, false, 0 }
};


//-----------------------------------------------------------------------------
// Logic for Symbolic mode
//
// The general test for a symbolic view from a schematic cell is
// CDs::symbolicRep(0) returning nonzero.  The CDc argument can
// override the status of the master.  The symbolic rep CDs always has
// the CDs_SYMBLC flag set, and is never in the database directly.  It
// resides in the symbolic property of its owning cell.  The
// CDs::owner() method returns a pointer to the owning cell if the
// cell is symbolic, null otherwise.  A symbolic cell always has the
// CDs_SYMBOLIC flag set, other cells never do.
//
// In general, if a CDs ia passed to a function, the function will
// do its operation on that CDs, and make no assumption about the type
// of CDs that "should" be passed.  It is always the caller's
// responsibility to pass the correct CDs.  This meaans that there is
// no guessing about what the caller intended to pass, but a function
// may issue an error or warning on a type it can't handle.  Some
// functions, like all property manipulation functions, pass through
// to the owner if called from a symbolic cell.
//
// Instances can not be added to symbolic cells, and symbolic cells
// can not be instantiated directly.  Symbolic cells don't have
// properties, all property operations pass through to the owning
// cell.  Operations to the modification counts pass through to the
// owner.  All flags except CDs_ELECTR, CDs_SYMBLC, and CDs_BBVALID pass
// through to the owner.  The owner stores and manipulates the
// masterRefs, node, and group data.
//
// Pointers to symbolic cells never occur in CD objects such as the
// main database, CDm, CDcbin, CallDesc.
//
// The DSP functions like CurCell, WindowDesc::CurCellDesc.  etc. 
// return the symbolic rep if it is being shown in the associated
// window.  If true is passed as the hidden arg to these funcs, then
// the schematic owning cell is returned instead.
//
// At the top level in a window, the "current cell" is displayed as
// symbolic if
// 1) The database (schematic) cell has an active symbolic
//    representation.
// 2) It is not the master of a CDc currently being considered, that
//    deactivates the symbolic view (E.g., in a Push).
// 3) It is not the top cell in a window with no_top_symbolic set.
//
//-----------------------------------------------------------------------------


//------------------------------------------------------------------------
// CDs::prptyLabelPatch is a bottleneck, this cache speeds things up.  It
// should be enabled while reading in electrical mode.
//
struct CDlabelCache
{
    CDlabelCache(const CDs *sd)
        {
            lc_sdesc = sd;
            lc_tab = new SymTab(false, false);
        }

    ~CDlabelCache()
        {
            SymTabGen gen(lc_tab, true);
            SymTabEnt *h;
            while ((h = gen.next()) != 0) {
                delete (SymTab*)h->stData;
                delete h;
            }
            delete lc_tab;
        }

    void add(CDpfxName name, unsigned long n, CDc *cd)
        {
            if (!name)
                return;
            SymTab *ntab = (SymTab*)lc_tab->get((unsigned long)name);
            if (ntab == (SymTab*)ST_NIL) {
                ntab = new SymTab(false, false);
                lc_tab->add((unsigned long)name, ntab, false);
            }
            ntab->add(n, cd, false);
        }

    CDc *find(CDpfxName name, unsigned long n) const
        {
            if (!name)
                return (0);
            SymTab *ntab = (SymTab*)lc_tab->get((unsigned long)name);
            if (ntab == (SymTab*)ST_NIL)
                return (0);
            CDc *cd = (CDc*)ntab->get(n);
            if (cd != (CDc*)ST_NIL)
                return (cd);
            return (0);
        }

    const CDs *celldesc()       const { return (lc_sdesc); }

private:
    const CDs *lc_sdesc;
    SymTab *lc_tab;
};


bool
cCD::EnableLabelPatchCache(bool b)
{
    bool tmp = cdUseLabelCache;
    cdUseLabelCache = b;
    if (!b) {
        delete cdLabelCache;
        cdLabelCache = 0;
    }
    return (tmp);
}


CDlabelCache *
cCD::GetLabelCache(const CDs *sdesc)
{
    if (cdUseLabelCache) {
        if (cdLabelCache) {
            if (cdLabelCache->celldesc() != sdesc) {
                delete cdLabelCache;
                cdLabelCache = 0;
            }
        }
        if (!cdLabelCache) {
            cdLabelCache = new CDlabelCache(sdesc);
            CDm_gen mgen(sdesc, GEN_MASTERS);
            for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
                CDc_gen cgen(mdesc);
                for (CDc *cdesc = cgen.c_first(); cdesc;
                        cdesc = cgen.c_next()) {
                    CDp_name *pn = (CDp_name*)cdesc->prpty(P_NAME);
                    if (!pn)
                        continue;
                    cdLabelCache->add(pn->name_string(), pn->number(), cdesc);
                }
            }
        }
        return (cdLabelCache);
    }
    return (0);
}


CDc *
cCD::LabelCacheFind(const CDlabelCache *cache, CDpfxName name, int num)
{
    if (cache)
        return (cache->find(name, num));
    return (0);
}
// End of sCD functions.


// CDgenHierDn_s functions.  This is a generator for traversing
// downward into the subcells.  It is ok to call on a recursive
// hierarchy.

void
CDgenHierDn_s::init(CDs *sdesc, int md)
{
    if (!sdesc) {
        hd_dp = -1;
        hd_tab = 0;
        return;
    }
    if (md < 0)
        md = CDMAXCALLDEPTH + 1;
    hd_sdescs[0] = sdesc;
    hd_generators[0].init(md > 0 ? sdesc->masters() : 0, false);
    hd_dp = 0;
    hd_maxdp = md;
    hd_tab = new ptrtab_t;
    hd_tab->add(sdesc);
}


CDs *
CDgenHierDn_s::next(bool *err)
{
    if (err)
        *err = false;
    for (;;) {
        if (hd_dp < 0)
            return (0);
        CDm *m = hd_generators[hd_dp].next();
        if (!m) {
            hd_dp--;
            return (hd_sdescs[hd_dp+1]);
        }

        CDs *msdesc = m->celldesc();
        if (!msdesc)
            continue;
        if (!m->hasInstances())
            continue;
        if (hd_tab->find(msdesc))
            continue;
        hd_dp++;
        if (hd_dp == CDMAXCALLDEPTH) {
            if (err)
                *err = true;
            return (0);
        }
        hd_tab->add(msdesc);
        hd_sdescs[hd_dp] = msdesc;
        hd_generators[hd_dp].init(
            hd_dp < hd_maxdp ? msdesc->masters() : 0, false);
    }
}
// End of CDgenHierDn_s functions.


// CDgenHierUp_s functions.  This is a generator for traversing upward
// into the parent cells.

void
CDgenHierUp_s::init(CDs *sdesc, int md)
{
    if (!sdesc) {
        hu_dp = -1;
        hu_tab = 0;
        return;
    }
    if (md < 0)
        md = CDMAXCALLDEPTH + 1;
    hu_sdescs[0] = sdesc;
    hu_generators[0].init(md > 0 ? sdesc->masterRefs() : 0, true);
    hu_dp = 0;
    hu_maxdp = md;
    hu_tab = new ptrtab_t;
}


CDs *
CDgenHierUp_s::next(bool *err)
{
    if (err)
        *err = false;
    for (;;) {
        if (hu_dp < 0)
            return (0);
        CDm *m = hu_generators[hu_dp].next();
        if (!m) {
            hu_dp--;
            hu_tab->add(hu_sdescs[hu_dp+1]);
            return (hu_sdescs[hu_dp+1]);
        }
        CDs *msdesc = m->parent();
        if (!msdesc)
            continue;
        if (hu_tab->find(msdesc))
            continue;
        hu_dp++;
        if (hu_dp == CDMAXCALLDEPTH) {
            if (err)
                *err = true;
            return (0);
        }
        hu_sdescs[hu_dp] = msdesc;
        hu_generators[hu_dp].init(
            hu_dp < hu_maxdp ? msdesc->masterRefs() : 0, true);
    }
}
// End of CDgenHierUp_s functions.


// Called by the archive readers.
//
void
CDs::setPCellFlags()
{
    if (prpty(XICP_PC_PARAMS)) {
        if (prpty(XICP_PC_SCRIPT))
            setPCell(true, true, false);
        else {
            CDp *pp = prpty(XICP_PC);
            if (pp) {
                // The property string for OA imports is in the format
                // "<libname><cellname><viewname>".  For Xic native,
                // this is the cell name, or possibly a file path(?).
                if (pp->string() && *pp->string() == '<')
                    setPCell(true, false, true);
                else
                    setPCell(true, false, false);
            }
        }
    }
}


unsigned int
CDs::getFlags() const
{
    CDs *sd = owner();
    if (sd) {
        // Assert the flags kept in the symbolic rep.
        unsigned int f = sd->sStatus & ~CDs_FILETYPE_MASK;
        unsigned int mask = CDs_BBVALID | CDs_SYMBOLIC;
        f &= ~mask;
        f |= mask & sStatus;
        return (f);
    }
    return (sStatus & ~CDs_FILETYPE_MASK);
}


void
CDs::setFlags(unsigned int f)
{
    CDs *sd = owner();
    if (sd) {
        // Deal with the two flags saved in the symbolic rep.
        bool bbvalid = f & CDs_BBVALID;
        unsigned int mask = CDs_BBVALID | CDs_SYMBOLIC;
        f &= ~mask;
        f |= sd->sStatus & CDs_BBVALID;
        sd->sStatus =
            (sd->sStatus & CDs_FILETYPE_MASK) | (f & ~CDs_FILETYPE_MASK);
        setBBvalid(bbvalid);
    }
    else
        sStatus = (sStatus & CDs_FILETYPE_MASK) | (f & ~CDs_FILETYPE_MASK);
}


// This is the "destroy" function - the CDs struct does not have its own
// C++ destructor due to issues with the memory manager and subclassing.
// The parent class CDdb destructor calls this function.
//
// If reuse is true, keep the desc and links intact, but free any
// objects.
//
void
CDs::clear(bool reuse)
{
    if (isImmutable() && reuse)
        return;

    // When (for example) destroying the extraction structs, we can
    // run across calls to CDc::masterCell(), which, if the master has
    // already been freed, will be regenerated with an empty cell.  We
    // explicitly prevent this from happening here.  When this is in
    // effect, code that calls CDc::masterCell() must test for a null
    // return and do something sensible.  This function is actually
    // recursive, so we use a count rather than a flag.
    //
    CDcdb()->incNoInsertNew();

    bool non_symbolic = !isSymbolic();
    if (non_symbolic) {
        if (sName && !CDcdb()->isNoCellCallback()) {
            // This zeros front-end references to this cell, deletes rulers,
            // and deletes the groups/nodemap.
            CD()->ifInvalidateCell(this);
        }
    }

    // Free property list
    prptyFreeList();

    if (!reuse)
        reflect_bad_BB();

    // Free objects
    db_clear();

    if (!reuse && !isElectrical()) {
        // Destroy the pins, have to be careful since the destructor
        // will remove from the list.

        while (Uhy.sPins) {
            delete Uhy.sPins->term();
            // Uhy.sPins has been advanced to next.
        }
    }

    if (!reuse && non_symbolic) {
        // Unlink the master desc reference list.
        CDm_gen gen(this, GEN_MASTER_REFS);
        for (CDm *m = gen.m_first(); m; m = gen.m_next())
            m->unlinkRef();
        if (sMasterRefs & 1)
            delete (ptable_t<CDm>*)(sMasterRefs & ~1);
        sMasterRefs = 0;
    }

    // db_clear() has already cleared the masters table
    if (sMasters & 1)
        delete (itable_t<CDm>*)(sMasters & ~1);
    sMasters = 0;

    if (isElectrical()) {
        delete [] getHY();
        setHY(0);
    }
    setFileName((CDarchiveName)0);

    if (reuse) {
        sBB = CDnullBB;
        sStatus = (CDs_BBVALID | (sStatus & CDs_ELECTR));
        // Above initializes filetype and mod count.
    }
    if (!reuse && sName && non_symbolic) {
        CDs *sx = CDcdb()->unlinkCell(this);

        // The sx is not null if the cell was in the main database,
        // Even if named, it might not be in the database (the
        // extraction system uses such cells).

        if (sx && !CDcdb()->isNoCellCallback()) {
            // If the counterpart cell is also gone, invalidate the name.
            if (isElectrical()) {
                if (!CDcdb()->findCell(sName, Physical))
                    CD()->ifInvalidateSymbol(sName);
            }
            else {
                if (!CDcdb()->findCell(sName, Electrical))
                    CD()->ifInvalidateSymbol(sName);
            }
        }
    }

    // Don't return before calling this!
    CDcdb()->decNoInsertNew();
}


// Record a modification.
//
void
CDs::incModified()
{
    if (isImmutable())
        return;

    CDs *sd = owner();
    if (sd) {
        sd->incModified();
        return;
    }

    unsigned int mcnt = getMcnt();
    if (mcnt != 0xffff)
        mcnt++;
    setMcnt(mcnt);
    CD()->ifCellModified(this);
}


void
CDs::decModified()
{
    if (isImmutable())
        return;

    CDs *sd = owner();
    if (sd) {
        sd->decModified();
        return;
    }
    unsigned int mcnt = getMcnt();
    if (mcnt != 0xffff && mcnt > 0)
        mcnt--;
    setMcnt(mcnt);
}


bool
CDs::isModified() const
{
    CDs *sd = owner();
    if (sd)
        return (sd->isModified());
    return (db_mod_count);
}


void
CDs::clearModified()
{
    CDs *sd = owner();
    if (sd) {
        sd->clearModified();
        return;
    }
    setMcnt(0);
}


// Return true is the cell is empty.  Electrical cells are not considered
// empty if certain properties exist.  If an argument is given, consider
// only objects on that layer.
//
bool
CDs::isEmpty(const CDl *ld) const
{
    if (isChdRef())
        return (false);
    if (isPCellSuperMaster())
        return (false);

    // Pass through symbolic cells to owner.
    CDs *tsd = owner();
    if (tsd)
        return (tsd->isEmpty(ld));

    if (isElectrical() && !ld) {
        for (CDp *pd = prptyList(); pd; pd = pd->next_prp()) {
            if (pd->value() == P_NODE || pd->value() == P_BRANCH ||
                    pd->value() == P_SYMBLC)
                return (false);
        }
    }
    if (sMasters) {
        if (sMasters & 1) {
            itable_t<CDm> *t = (itable_t<CDm>*)(sMasters & ~1);
            if (!t || !t->allocated())
                return (db_is_empty(ld));
        }
        return (false);
    }
    return (db_is_empty(ld));
}


// Return true if this is instantiated by another cell.
//
bool
CDs::isSubcell() const
{
    unsigned long master_refs = masterRefs();
    if (!master_refs)
        return (false);
    if (master_refs & 1) {
        ptable_t<CDm> *t = (ptable_t<CDm>*)(master_refs & ~1);
        if (!t || !t->allocated())
            return (false);
    }
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        if (m->hasInstances())
            return (true);
    }
    return (false);
}


// Return true if a cell in the hierarchy has its modified flag set.
//
bool
CDs::isHierModified() const
{
    if (isModified())
        return (true);
    CDm_gen mgen(this, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        if (mdesc->hasInstances() && mdesc->celldesc()->isHierModified())
            return (true);
    }
    return (false);
}


// Return true if any cell in the sd hierarchy is among this and its
// instantiators.  Adding sd to this would create a recursive
// hierarchy in this case.
//
bool
CDs::isRecursive(CDs *sd)
{
    if (!sd)
        return (false);
    CDgenHierUp_s pgen(this);
    CDs *sp;
    while ((sp = pgen.next()) != 0) {
        CDgenHierDn_s sg(sd);
        CDs *s;
        while ((s = sg.next()) != 0) {
            if (s == sp)
                return (true);
        }
    }
    return (false);
}


CDp_sym *
CDs::symbolicPrpty(const CDc *cdesc) const
{
    if (isElectrical() && !isSymbolic()) {
        CDp_sym *ps = (CDp_sym*)prpty(P_SYMBLC);
        if (ps && ps->active() && ps->symrep()) {
            if (cdesc) {
                if (cdesc == CD_NO_SYMBOLIC)
                    return (0);
                CDp_sym *pcs = (CDp_sym*)cdesc->prpty(P_SYMBLC);
                if (pcs && !pcs->active())
                    return (0);
            }
            return (ps);
        }
    }
    return (0);
}


// Return the symbolic representation if structure is actively symbolic.
//
CDs *
CDs::symbolicRep(const CDc *cdesc) const
{
    if (isElectrical() && !isSymbolic()) {
        CDp_sym *ps = (CDp_sym*)prpty(P_SYMBLC);
        if (ps && ps->active()) {
            if (cdesc) {
                if (cdesc == CD_NO_SYMBOLIC)
                    return (0);
                CDp_sym *pcs = (CDp_sym*)cdesc->prpty(P_SYMBLC);
                if (pcs && !pcs->active())
                    return (0);
            }
            return (ps->symrep());
        }
    }
    return (0);
}


// Compute the area covered by ldesc, recursively if recurse is true.
// This *does not* account for overlapping objects.
//
double
CDs::area(const CDl *ldesc, bool recurse) const
{
    double d = 0.0;
    if (!ldesc)
        return (d);
    if (recurse) {
        CDm_gen mgen(this, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            CDs *msdesc = mdesc->celldesc();
            if (!msdesc)
                continue;
            double a = msdesc->area(ldesc, true);
            CDc_gen cgen(mdesc);
            for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
                if (!cdesc->is_normal())
                    continue;
                CDap ap(cdesc);
                CDtx tx(cdesc);
                d += ap.nx*ap.ny*tx.magn*tx.magn*a;
            }
        }
    }
    CDg gdesc;
    gdesc.init_gen(this, ldesc);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (odesc->type() == CDLABEL)
            continue;
        if (!odesc->is_normal())
            continue;
        d += odesc->area();
    }
    return (d);
}


// Clear all objects on ld.  If Undoable is true, do it the slow but
// reversable way.
//
void
CDs::clearLayer(CDl *ld, bool Undoable)
{
    if (isImmutable())
        return;
    if (!ld)
        return;
    if (!Undoable) {
        // Clear objects on this layer in the undo list and selection
        // queue.
        CD()->ifInvalidateLayerContent(this, ld);
        db_clear_layer(ld);
    }
    else {
        CDg gdesc;
        gdesc.init_gen(this, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0)
            CD()->ifRecordObjectChange(this, odesc, 0);
    }
}


// Compute bounding box, both main cell and symbolic rep if it exists.
//
bool
CDs::computeBB()
{
    if (isChdRef())
        return (true);

    CDs *sd = owner();
    if (!sd)
        sd = this;
    CDs *sdy = sd->symbolicRep(0);

    if (!sd->isBBvalid()) {
        sd->db_bb(&sd->sBB);
        if (sd->isElectrical()) {
            // Expand BB to enclose all terminals, needed only for
            // virtual and bus terminals.
            CDp_snode *pn = (CDp_snode*)sd->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                int x, y;
                pn->get_schem_pos(&x, &y);
                sd->sBB.add(x, y);
            }
            CDp_bsnode *pb = (CDp_bsnode*)sd->prpty(P_BNODE);
            for ( ; pb; pb = pb->next()) {
                int x, y;
                pb->get_schem_pos(&x, &y);
                sd->sBB.add(x, y);
            }
        }
        sd->setBBvalid(true);

        if (!(sd->sBB <= CDinfiniteBB)) {
            CD()->ifPrintCvLog(IFLOG_WARN,
                "The bounding box of %s extends beyond \"infinity\",\n"
                "scaling problem?  Display and other things won't work right!",
                cellname()->string());
        }
    }

    if (sdy && !sdy->isBBvalid()) {
        sdy->db_bb(&sdy->sBB);
        // Expand BB to enclose all terminals, needed only for
        // virtual and bus terminals.
        CDp_snode *pn = (CDp_snode*)sdy->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pn->get_pos(ix, &x, &y))
                    break;
                sdy->sBB.add(x, y);
            }
        }
        CDp_bsnode *pb = (CDp_bsnode*)sdy->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pb->get_pos(ix, &x, &y))
                    break;
                sdy->sBB.add(x, y);
            }
        }
        sdy->setBBvalid(true);
    }
    return (true);
}


// Check and set the bounding boxes of all cells in the hierarchy, and
// also check/set master desc cell pointers and a few other things.
//
bool
CDs::fixBBs(ptrtab_t *tab)
{
    bool ret = true;
    bool local_tab = false;
    if (!tab) {
        tab = new ptrtab_t;
        local_tab = true;
    }
    if (!tab->find(this)) {
        ret = fix_bb(tab, 0);
        tab->add(this);
    }
    if (local_tab)
        delete tab;

    return (ret);
}


// Make sure that all instances are in the database, and test for
// duplicates.  Return false if an instance was not in the database.
//
bool
CDs::checkInstances()
{
    bool ok = true;
    CDol *dups = 0;
    CDm_gen mgen(this, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            // "can't happen", handle elsewhere
            continue;

        // All instances should be in the database.
        CDc_gen cgen(mdesc);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {

            if (!c->in_db()) {
                ok = false;
                CD()->ifPrintCvLog(IFLOG_WARN,
                    "internal error, instance of %s not in %s database.",
                    mdesc->cellname()->string(), cellname()->string());
                continue;
            }
            if (CD()->IsReading() && CD()->DupCheckMode() != cCD::DupNoTest) {
                if (db_check_coinc(c)) {
                    const char *what = "kept";
                    if (CD()->DupCheckMode() == cCD::DupRemove) {
                        dups = new CDol(c, dups);
                        what = "removed";
                    }
                    char tbf[256];
                    sprintf(tbf, "coincident instance of %s (%s)",
                        mdesc->cellname()->string(), what);
                    CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s %d,%d].", tbf,
                        cellname()->string(), c->oBB().left, c->oBB().top);
                }
            }
        }
    }
    while (dups) {
        CDol *ox = dups;
        dups = dups->next;
        unlink(ox->odesc, false);
        delete ox;
    }
    return (ok);
}


// Return true if there is an underlying connection point.
//
bool
CDs::findNode(int x, int y, int *node)
{
    if (!isElectrical()) {
        if (node)
            *node = -1;
        return (false);
    }

    BBox AOI(x, y, x, y);

    // First check the devices.
    CDg gdesc;
    gdesc.init_gen(this, CellLayer(), &AOI);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (!odesc->is_normal())
            continue;
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int px, py;
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (px == x && py == y) {
                    if (node)
                        *node = pn->enode();
                    return (true);
                }
            }
        }
    }

    // Now check active wires.
    CDsLgen gen(this);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(this, ld, &AOI);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE)
                continue;
            if (!wd->is_normal())
                continue;
            CDp_node *pn = (CDp_node*)wd->prpty(P_NODE);
            if (pn) {
                const Point *pts = wd->points();
                int num = wd->numpts();
                for (int i = 0; i < num; i++) {
                    if (pts[i].x == x && pts[i].y == y) {
                        if (node)
                            *node = pn->enode();
                        return (true);
                    }
                }
            }
        }
    }
    if (node)
        *node = -1;
    return (false);
}


// Return true if there is a device terminal or active wire vertex at
// x, y not belonging to wrdesc, false otherwise.
//
bool
CDs::checkVertex(int x, int y, CDw *wrdesc)
{
    if (!isElectrical())
        return (false);

    BBox AOI(x, y, x, y);

    // First check the devices.
    CDg gdesc;
    gdesc.init_gen(this, CellLayer(), &AOI);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (!odesc->is_normal())
            continue;
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int px, py;
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (px == x && py == y)
                    return (true);
            }
        }
        CDp_bcnode *pb = (CDp_bcnode*)odesc->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int px, py;
                if (!pb->get_pos(ix, &px, &py))
                    break;
                if (px == x && py == y)
                    return (true);
            }
        }
    }

    // Now check active wires.
    CDsLgen gen(this);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(this, ld, &AOI);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE)
                continue;
            if (!wd->is_normal())
                continue;
            if (wd == wrdesc)
                continue;
            const Point *pts = wd->points();
            int num = wd->numpts();
            for (int i = 0; i < num; i++) {
                if (pts[i].x == x && pts[i].y == y)
                    return (true);
            }
        }
    }

    // Check cell contact points.
    CDp_snode *pn = (CDp_snode*)prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        int px, py;
        pn->get_schem_pos(&px, &py);
        if (px == x && py == y)
            return (true);
    }
    return (false);
}


// Return true if there is a connection point at px,py within slop,
// move the pointers to the exact value.  If no_fterms, don't check
// the cell connection points.
//
bool
CDs::checkVertex(int *px, int *py, int slop, bool no_fterms)
{
    if (!isElectrical())
        return (false);

    BBox AOI(*px, *py, *px, *py);
    AOI.bloat(slop);

    // First check the devices.
    CDg gdesc;
    gdesc.init_gen(this, CellLayer(), &AOI);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (!odesc->is_normal())
            continue;
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pn->get_pos(ix, &x, &y))
                    break;
                if (AOI.intersect(x, y, true)) {
                    *px = x;
                    *py = y;
                    return (true);
                }
            }
        }
        CDp_bcnode *pb = (CDp_bcnode*)odesc->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pb->get_pos(ix, &x, &y))
                    break;
                if (AOI.intersect(x, y, true)) {
                    *px = x;
                    *py = y;
                    return (true);
                }
            }
        }
    }

    // Now check active wires.
    CDsLgen gen(this);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(this, ld, &AOI);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE)
                continue;
            if (!wd->is_normal())
                continue;
            const Point *pts = wd->points();
            int num = wd->numpts();
            for (int i = 0; i < num; i++) {
                if (AOI.intersect(&pts[i], true)) {
                    *px = pts[i].x;
                    *py = pts[i].y;
                    return (true);
                }
            }
        }
    }

    if (!no_fterms) {
        // Check cell contact points.
        CDp_snode *pn = (CDp_snode*)prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            int x, y;
            pn->get_schem_pos(&x, &y);
            if (AOI.intersect(x, y, true)) {
                *px = x;
                *py = y;
                return (true);
            }
        }
    }
    return (false);
}


// Update the BB of all instances of sdesc.  Does not check that the
// BB of sdesc is correct.
// Returns false on error.
//
bool
CDs::reflect()
{
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        if (!m->reflect())
            return (false);
    }
    return (true);
}


// Duplicate a structure.  If an arg is passed, duplicate into that
// structure, otherwise create a new one.
//
// If a new cell is created, it has no name, and is not linked into
// any table.
//
CDs *
CDs::cloneCell(CDs *newdesc) const
{
    if (!newdesc)
        newdesc = new CDs(0, isElectrical() ? Electrical : Physical);
    if (isSymbolic())
        newdesc->setSymbolic(true);
    if (isBBsubng())
        newdesc->setBBsubng(true);

    CD()->ifUpdateNodes(this);
    for (CDp *pdesc = sPrptyList; pdesc; pdesc = pdesc->next_prp()) {
        if (prpty_reserved(pdesc->value()))
            continue;
        CDp *pnew = newdesc->prptyCopy(pdesc);
        if (!pnew)
            continue;
        pnew->set_next_prp(newdesc->sPrptyList);
        newdesc->sPrptyList = pnew;
        if (!isElectrical())
            continue;
        pnew->bind(0);
        CDla *olabel = pdesc->bound();
        if (olabel) {
            // mutual inductor property
            CDla *nlabel = (CDla*)olabel->dup(newdesc);
            pnew->bind(nlabel);
            nlabel->link(newdesc, 0, 0);
        }
    }

    int tflags = newdesc->getFlags();
    CDp_nmut *pm = (isElectrical() ? (CDp_nmut*)newdesc->prpty(P_NEWMUT) : 0);

    CDm_gen mgen(this, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            CDc *cnew = (CDc*)cdesc->dup(newdesc);
            if (isElectrical()) {
                // Correct pointers for mutual inductors
                for (CDp_nmut *pmm = pm; pmm; pmm = pmm->next()) {
                    if (pmm->l1_dev() == cdesc)
                        pmm->set_l1_dev(cnew);
                    else if (pmm->l2_dev() == cdesc)
                        pmm->set_l2_dev(cnew);
                }
                // Correct pointers for hypertext references, which
                // are added to the (new) list during property copy
                hyEnt **hy = getHY();
                if (hy) {
                    for (int i = 0; hy[i]; i++) {
                        for (hyParent *p = hy[i]->parent(); p; p = p->next()) {
                            if (p->cdesc() == cdesc)
                                p->set_cdesc(cnew);
                        }
                    }
                }
            }
        }
    }
    CDl *ldesc;
    CDsLgen gen(this);
    while ((ldesc = gen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(this, ldesc);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (odesc->type() == CDLABEL && isElectrical()) {
                CDp_lref *prf = (CDp_lref*)odesc->prpty(P_LABRF);
                if (prf && (prf->devref() || prf->wireref()))
                    // These are already added.
                    continue;
            }
            odesc->dup(newdesc);
                
        }
    }
    newdesc->setFlags(tflags);
    newdesc->setLibrary(false);
    newdesc->setImmutable(false);
    return (newdesc);
}


// The cell terminals are owned by CDs (unlike instance terminals
// which are owned by electrical properties).  They should be created
// with this function only.
//
CDsterm *
CDs::findPinTerm(CDnetName nm, bool create)
{
    if (!nm)
        return (0);
    for (CDpin *p = Uhy.sPins; p; p = p->next()) {
        if (p->term()->name() == nm)
            return (p->term());
        if (!p->next() && create) {
            p->set_next(new CDpin(new CDsterm(this, nm), 0));
            return (p->next()->term());
        }
    }
    if (create) {
        Uhy.sPins = new CDpin(new CDsterm(this, nm), 0);
        return (Uhy.sPins->term());
    }
    return (0);
}


// Remove t from the cell's list, called from the CDsterm destructor.
//
bool
CDs::removePinTerm(CDsterm *t)
{
    if (!t)
        return (false);
    CDpin *pp = 0;
    for (CDpin *p = Uhy.sPins; p; p = p->next()) {
        if (p->term() == t) {
            if (pp)
                pp->set_next(p->next());
            else
                Uhy.sPins = p->next();
            delete p;
            return (true);
        }
        pp = p;
    }
    return (false);
}


// Create a box.
//
CDerrType
CDs::makeBox(CDl *ldesc, const BBox *pBB, CDo **pointer, bool internal)
{
    if (pointer)
        *pointer = 0;
    if (!ldesc) {
        CD()->Error(CDbadLayer, cellname()->string());
        return (CDbadLayer);
    }
    if (!pBB || !pBB->valid()) {
        CD()->Error(CDbadBox, cellname()->string());
        return (CDbadBox);
    }
    CDo *odesc = new CDo(ldesc, pBB);
    if (internal)
        odesc->set_state(CDInternal);
    if (!insert(odesc)) {
        delete odesc;
        return (CDfailed);
    }
    if (pointer)
        *pointer = odesc;

    if (CD()->IsReading() && CD()->DupCheckMode() != cCD::DupNoTest) {
        if (db_check_coinc(odesc)) {
            const char *what = "kept";
            if (CD()->DupCheckMode() == cCD::DupRemove)
                what = "removed";

            char tbf[64];
            int c = CD()->CheckCoincErrs();
            if (c < 0) {
                snprintf(tbf, 64, "coincident box (%s)", what);
                CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s %d,%d %s].", tbf,
                    cellname()->string(), odesc->oBB().left, odesc->oBB().top,
                    odesc->ldesc()->name());
            }
            else if (c == 0) {
                snprintf(tbf, 64, "more coincident objects (%s)", what);
                CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s].", tbf,
                    cellname()->string());
            }
            if (CD()->DupCheckMode() == cCD::DupRemove) {
                unlink(odesc, false);
                if (pointer)
                    *pointer = 0;
            }
        }
    }
    return (CDok);
}


// Create a box, copy properties and add to undo list.
//
CDo *
CDs::newBox(CDo *oldobj, const BBox *pBB, CDl *ldesc, CDp *prps)
{
    CDo *newo;
    if (makeBox(ldesc, pBB, &newo) != CDok)
        return (0);
    if (newo) {
        CD()->ifRecordObjectChange(this, oldobj, newo);
        if (prps)
            newo->prptyAddCopyList(prps);
    }
    return (newo);
}


// Create a box, copy properties and add to undo list.
//
CDo *
CDs::newBox(CDo *oldobj, int l, int b, int r, int t, CDl *ldesc, CDp *prps)
{
    BBox tBB(l, b, r, t);
    tBB.fix();
    CDo *newo;
    if (makeBox(ldesc, &tBB, &newo) != CDok)
        return (0);
    if (newo) {
        CD()->ifRecordObjectChange(this, oldobj, newo);
        if (prps)
            newo->prptyAddCopyList(prps);
    }
    return (newo);
}


// Create a polygon.  Warning:  the poly->points pointer is zeroed, as
// the points list is owned by the database (or freed on error).
// The points list *must* be heap-allocated, no copying is done.
//
CDerrType
CDs::makePolygon(CDl *ldesc, Poly *poly, CDpo **pointer, int *pchk_flags,
    bool internal)
{
    if (pointer)
        *pointer = 0;
    if (pchk_flags)
        *pchk_flags = 0;
    if (!ldesc) {
        if (poly) {
            delete [] poly->points;
            poly->points = 0;
        }
        CD()->Error(CDbadLayer, cellname()->string());
        return (CDbadLayer);
    }
    if (!poly || poly->numpts < 1 || !poly->points) {
        if (poly) {
            delete [] poly->points;
            poly->points = 0;
        }
        CD()->Error(CDbadPolygon, cellname()->string());
        return (CDbadPolygon);
    }
    // Always include closure point in path.
    int numpts = poly->numpts;
    if (poly->points[0].x != poly->points[numpts-1].x ||
            poly->points[0].y != poly->points[numpts-1].y) {
        // reallocate larger array
        Point *pts = new Point[numpts+1];
        int i;
        for (i = 0; i < numpts; i++)
            pts[i] = poly->points[i];
        pts[i] = poly->points[0];
        delete [] poly->points;
        poly->points = pts;
        numpts++;
        poly->numpts = numpts;
    }
    Point::removeDups(poly->points, &poly->numpts);

    if (!CD()->IsNotStrict()) {
        if (!poly->check_quick()) {
            delete [] poly->points;
            poly->points = 0;
            CD()->Error(CDbadPolygon, cellname()->string());
            return (CDbadPolygon);
        }
        if (!CD()->IsNoPolyCheck()) {
            int ret = poly->check_poly(PCHK_ALL);
            if (ret & (PCHK_NVERTS | PCHK_OPEN)) {
                delete [] poly->points;
                poly->points = 0;
                CD()->Error(CDbadPolygon, cellname()->string());
                return (CDbadPolygon);
            }
            if (pchk_flags)
                *pchk_flags = ret;
        }
    }

    CDpo *pdesc = new CDpo(ldesc, poly);
    poly->points = 0;

    if (internal)
        pdesc->set_state(CDInternal);
    if (!insert(pdesc)) {
        delete pdesc;
        return (CDfailed);
    }
    if (pointer)
        *pointer = pdesc;

    if (CD()->IsReading() && CD()->DupCheckMode() != cCD::DupNoTest) {
        if (db_check_coinc(pdesc)) {
            const char *what = "kept";
            if (CD()->DupCheckMode() == cCD::DupRemove)
                what = "removed";

            char tbf[64];
            int c = CD()->CheckCoincErrs();
            if (c < 0) {
                snprintf(tbf, 64, "coincident polygon (%s)", what);
                CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s %d,%d %s].", tbf,
                    cellname()->string(), pdesc->points()->x,
                    pdesc->points()->y, pdesc->ldesc()->name());
            }
            else if (c == 0) {
                snprintf(tbf, 64, "more coincident objects (%s)", what);
                CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s].", tbf,
                    cellname()->string());
            }
            if (CD()->DupCheckMode() == cCD::DupRemove) {
                unlink(pdesc, false);
                if (pointer)
                    *pointer = 0;
            }
        }
    }
    return (CDok);
}


// Create a polygon, copy properties and add to undo list.  If Copy is
// true, the poly->points list is copied, and the passed Poly is
// returned untouched.  Otherwise, the poly->points list is zeroed, and
// the description of makePoly applies.
//
CDpo *
CDs::newPoly(CDo *oldobj, Poly *poly, CDl *ldesc, CDp *prps, bool Copy)
{
    CDpo *newo;
    if (Copy) {
        if (!poly || poly->numpts < 1 || !poly->points) {
            CD()->Error(CDbadPolygon, cellname()->string());
            return (0);
        }
        Poly npoly(poly->numpts, Point::dup(poly->points, poly->numpts));
        if (makePolygon(ldesc, &npoly, &newo) != CDok)
            return (0);
    }
    else if (makePolygon(ldesc, poly, &newo) != CDok)
        return (0);
    if (newo) {
        CD()->ifRecordObjectChange(this, oldobj, newo);
        if (prps)
            newo->prptyAddCopyList(prps);
    }
    return (newo);
}


// Create a wire.  Warning:  the wire->points pointer is zeroed, as
// the points list is owned by the database (or freed on error).
// The points list *must* be heap-allocated, no copying is done.
//
CDerrType
CDs::makeWire(CDl *ldesc, Wire *wire, CDw **pointer, int *wchk_flags,
    bool internal)
{
    if (pointer)
        *pointer = 0;
    if (wchk_flags)
        *wchk_flags = 0;
    if (!ldesc) {
        if (wire) {
            delete [] wire->points;
            wire->points = 0;
        }
        CD()->Error(CDbadLayer, cellname()->string());
        return (CDbadLayer);
    }
    if (!wire || wire->numpts < 1 || !wire->points) {
        if (wire) {
            delete [] wire->points;
            wire->points = 0;
        }
        CD()->Error(CDbadWire, cellname()->string());
        return (CDbadWire);
    }

    Point::removeDups(wire->points, &wire->numpts);

    if (!CD()->IsNotStrict()) {
        if (wchk_flags) {
            Point *pts;
            int npts;
            unsigned int flags;
            if (!wire->toPoly(&pts, &npts, &flags)) {
                delete [] wire->points;
                wire->points = 0;
                CD()->Error(CDbadWire, cellname()->string());
                return (CDbadWire);
            }
            delete [] pts;
            if (isElectrical())
                flags &= (CDWIRE_CLIPFIX | CDWIRE_BIGPOLY);
            else
                flags &= (CDWIRE_ZEROWIDTH | CDWIRE_CLIPFIX | CDWIRE_BIGPOLY);
            *wchk_flags = flags;
        }
        else if (wire->numpts == 1 &&
                (wire->wire_style() == CDWIRE_FLUSH ||
                wire->wire_width() == 0)) {
            delete [] wire->points;
            wire->points = 0;
            CD()->Error(CDbadWire, cellname()->string());
            return (CDbadWire);
        }
    }

    CDw *wdesc = new CDw(ldesc, wire);
    wire->points = 0;

    if (internal)
        wdesc->set_state(CDInternal);
    if (!insert(wdesc)) {
        delete wdesc;
        return (CDfailed);
    }
    if (pointer)
        *pointer = wdesc;

    if (CD()->IsReading() && CD()->DupCheckMode() != cCD::DupNoTest) {
        if (db_check_coinc(wdesc)) {
            const char *what = "kept";
            if (CD()->DupCheckMode() == cCD::DupRemove)
                what = "removed";

            char tbf[64];
            int c = CD()->CheckCoincErrs();
            if (c < 0) {
                snprintf(tbf, 64, "coincident wire (%s)", what);
                CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s %d,%d %s].", tbf,
                    cellname()->string(), wdesc->points()->x,
                    wdesc->points()->y, wdesc->ldesc()->name());
            }
            else if (c == 0) {
                snprintf(tbf, 64, "more coincident objects (%s)", what);
                CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s].", tbf,
                    cellname()->string());
            }
            if (CD()->DupCheckMode() == cCD::DupRemove) {
                unlink(wdesc, false);
                if (pointer)
                    *pointer = 0;
            }
        }
    }
    return (CDok);
}


// Create a wire, copy properties and add to undo list.  If Copy is
// true, the wire->points list is copied, and the passed Wire is
// returned untouched.  Otherwise, the wire->points list is zeroed,
// and the description of makeWire applies.
//
CDw *
CDs::newWire(CDo *oldobj, Wire *wire, CDl *ldesc, CDp *prps, bool Copy)
{
    CDw *newo;
    if (Copy) {
        if (!wire || wire->numpts < 1 || !wire->points) {
            CD()->Error(CDbadWire, cellname()->string());
            return (0);
        }
        Wire nwire(wire->numpts, Point::dup(wire->points, wire->numpts),
            wire->attributes);
        if (makeWire((CDl*)ldesc, &nwire, &newo) != CDok)
            return (0);
    }
    else if (makeWire((CDl*)ldesc, wire, &newo) != CDok)
        return (0);
    if (newo) {
        CD()->ifRecordObjectChange(this, oldobj, newo);
        if (prps)
            newo->prptyAddCopyList(prps);
    }
    return (newo);
}


// Create a label.  Warning: the ladesc->label pointer is zeroed, as the
// label is owned by the database (or freed on error).
//
CDerrType
CDs::makeLabel(CDl *ldesc, Label *label, CDla **pointer, bool internal)
{
    if (pointer)
        *pointer = 0;
    if (!ldesc) {
        if (label) {
            hyList::destroy(label->label);
            label->label = 0;
        }
        CD()->Error(CDbadLayer, cellname()->string());
        return (CDbadLayer);
    }
    if (!label || !label->label) {
        CD()->Error(CDbadLabel, cellname()->string());
        return (CDbadLabel);
    }
    CDla *ladesc = new CDla(ldesc, label);
    label->label = 0;

    if (internal)
        ladesc->set_state(CDInternal);
    if (!insert(ladesc)) {
        delete ladesc;
        return (CDfailed);
    }
    if (pointer)
        *pointer = ladesc;

    if (CD()->IsReading() && CD()->DupCheckMode() != cCD::DupNoTest) {
        if (db_check_coinc(ladesc)) {
            const char *what = "kept";
            if (CD()->DupCheckMode() == cCD::DupRemove)
                what = "removed";

            char tbf[64];
            int c = CD()->CheckCoincErrs();
            if (c < 0) {
                snprintf(tbf, 64, "coincident label (%s)", what);
                CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s %d,%d %s].", tbf,
                    cellname()->string(), ladesc->oBB().left,
                    ladesc->oBB().top, ladesc->ldesc()->name());
            }
            else if (c == 0) {
                snprintf(tbf, 64, "more coincident objects (%s)", what);
                CD()->ifPrintCvLog(IFLOG_WARN, "%s [%s].", tbf,
                    cellname()->string());
            }
            if (CD()->DupCheckMode() == cCD::DupRemove) {
                unlink(ladesc, false);
                if (pointer)
                    *pointer = 0;
            }
        }
    }
    return (CDok);
}


// Create a label, copy properties and add to undo list.  If Copy is
// true, the label->label is copied, and the passed Label is returned
// untouched.  Otherwise, the label->label is zeroed, and the
// description of makeLabel applies.
//
CDla *
CDs::newLabel(CDo *oldobj, Label *label, CDl *ldesc, CDp *prps, bool Copy)
{
    CDla *newo;
    if (Copy) {
        if (!label || !label->label) {
            CD()->Error(CDbadLabel, cellname()->string());
            return (0);
        }
        hyList *tl = label->label;
        label->label = hyList::dup(label->label);
        CDerrType ret = makeLabel(ldesc, label, &newo);
        label->label = tl;
        if (ret != CDok)
            return (0);
    }
    else if (makeLabel(ldesc, label, &newo) != CDok)
        return (0);
    if (newo) {
        CD()->ifRecordObjectChange(this, oldobj, newo);
        if (prps)
            newo->prptyAddCopyList(prps);
    }
    return (newo);
}


// Add a representation of the poly to the database, on layer ld.  If
// the poly can be represented as a box, a box object will be created
// instead of a polygon object.  If undoable is true, call the hooks
// to the undo list, and in addition attempt to merge it with existing
// objects if use_merge is true.  If tstk is given, transform the
// coordinates.  If oret is non-nil, and the undoable flag is NOT set,
// a pointer to the new object is returned on success.
//
CDerrType
CDs::addToDb(Poly &poly, CDl *ld, bool undoable, CDo **oret,
    const cTfmStack *tstk, bool use_merge)
{
    if (oret)
        *oret = 0;
    CDerrType ret = CDok;
    Poly po(poly);
    po.points = Point::dup_with_xform(po.points, tstk, po.numpts);

    if (po.is_rect()) {
        BBox tBB(po.points);
        delete [] po.points;
        CDo *newo;
        ret = makeBox(ld, &tBB, &newo);
        if (ret != CDok)
            Errs()->add_error("makeBox failed");
        else if (undoable) {
            CD()->ifRecordObjectChange(this, 0, newo);
            if (use_merge && !mergeBoxOrPoly(newo, true)) {
                Errs()->add_error("mergeBoxOrPoly failed");
                ret = CDfailed;
            }
        }
        else if (oret)
            *oret = newo;
    }
    else {
        CDpo *newo;
        ret = makePolygon(ld, &po, &newo);
        if (ret != CDok) {
            Errs()->add_error("makePolygon failed");
            delete [] po.points;
        }
        else if (undoable) {
            CD()->ifRecordObjectChange(this, 0, newo);
            if (use_merge && !mergeBoxOrPoly(newo, true)) {
                Errs()->add_error("mergeBoxOrPoly failed");
                ret = CDfailed;
            }
        }
        else if (oret)
            *oret = newo;
    }
    return (ret);
}


// Add the list to the database as for addToDb(Poly&, ...).  If
// olret is non-nil and the undoable flag is NOT set, return a list of
// the newly created objects.  This list can be non-empty if this
// function fails - it lists the objects successfully inserted.
//
CDerrType
CDs::addToDb(PolyList *plist, CDl *ld, bool undoable, CDol **olret,
    const cTfmStack *tstk, bool use_merge)
{
    if (olret)
        *olret = 0;
    CDerrType ret = CDok;
    for (PolyList *p = plist; p; p = p->next) {
        CDo *newo;
        ret = addToDb(p->po, ld, undoable, &newo, tstk, use_merge);
        if (ret != CDok)
            break;
        if (!undoable && olret)
            *olret = new CDol(newo, *olret);
    }
    return (ret);
}


// Instance creation.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cell opened ok
//    OIaborted     user aborted
//
OItype
CDs::makeCall(CallDesc *calldesc, const CDtx *tx, const CDap *ap,
    CDcallType omode, CDc **pointer)
{
    if (pointer)
        *pointer = 0;
    if (!calldesc) {
        Errs()->add_error("makeCall internal error, null call desc.");
        return (OIerror);
    }
    if (!calldesc->name()) {
        Errs()->add_error(
            "Null name in instance call encountered in cell %s.",
            cellname()->string());
        return (OIerror);
    }
    if (isImmutable()) {
        Errs()->add_error("attempt to add to immutable cell.");
        return (OIerror);
    }
    if (isSymbolic()) {
        Errs()->add_error(
            "Attempt to add instance to symbolic representation.");
        return (OIerror);
    }

    if (omode == CDcallNone) {
        // Placing an instance from the user interface, check for
        // recursion.  This is expensive so do it only for the first
        // placement, when there is no master desc for the name.

        if (calldesc->celldesc() && !findMaster(calldesc->name())) {
            if (isRecursive(calldesc->celldesc())) {
                Errs()->add_error(
                "Can't instantiate %s in %s, hierarchy would be recursive!",
                    calldesc->name()->string(), cellname()->string());
                return (OIerror);
            }
        }
    }

    CDs *msdesc = calldesc->celldesc();
    if (msdesc && msdesc->owner())
        msdesc = msdesc->owner();
    if (msdesc && msdesc->isPCellSuperMaster()) {
        // "can't happen"
        Errs()->add_error(
            "Can't directly instantiate a super-master (%s).",
            msdesc->cellname()->string());
        return (OIerror);
    }
    CDc *cdesc = new CDc(CellLayer());
    CDm *mdesc = insertMaster(calldesc->name());

    // Prior to 3.2.26, pcell sub-master instances had no properties,
    // so instances read from archives likely have no pcell
    // properties.  These will have to be added later when resolving
    // the "unread" masters (in cv_in::mark_references and
    // cFIO::FromNative).  Giving the properties to instances allows
    // the sub-master to be recreated when missing.

    if (mdesc->isNullCelldesc()) {
        if (!msdesc) {
            msdesc = CDcdb()->findCell(mdesc->cellname(), displayMode());
            if (!msdesc) {
                const char *cname = mdesc->cellname()->string();
                CDcbin cbin;
                if (FIO()->OpenLibCell(0, cname,
                        LIBdevice | LIBuser | LIBnativeOnly, &cbin) == OIok)
                    msdesc = cbin.celldesc(displayMode());
            }
            if (!msdesc) {
                // Create a new cell with the unread flag set.  These
                // should be resolved later.

                msdesc = CDcdb()->insertCell(mdesc->cellname()->string(),
                    displayMode());
                if (msdesc)
                    msdesc->setUnread(true);
            }
            if (!msdesc) {
                Errs()->add_error(
                    "makeCall: creation %s %s in %s failed.",
                    isElectrical() ? "electrical" : "physical",
                    calldesc->name(), cellname()->string());
                delete cdesc;
                return (OIerror);
            }
        }
        mdesc->linkRef(msdesc);
    }
    else
        msdesc = mdesc->celldesc();
    cdesc->setMaster(mdesc);

    // set instance transform
    CDattr at(tx, ap);
    if (tx) {
        cdesc->setPosX(tx->tx);
        cdesc->setPosY(tx->ty);
    }
    cdesc->setAttr(CD()->RecordAttr(&at));

    if (msdesc->isPCellSubMaster()) {
        // Copy the pcell properties.
        CDp *prp = msdesc->prpty(XICP_PC);
        cdesc->prptyAddCopy(prp);
        prp = msdesc->prpty(XICP_PC_PARAMS);
        cdesc->prptyAddCopy(prp);
    }
    else if (msdesc->isViaSubMaster()) {
        // Copy the standard via property.
        CDp *prp = msdesc->prpty(XICP_STDVIA);
        cdesc->prptyAddCopy(prp);
    }

    if ((!msdesc->isBBsubng() && msdesc->sBB.left != CDinfinity) ||
            (omode == CDcallNone && isElectrical())) {
        // Subcell has been read and has a good BB, or we need to
        // add/transform properties.
        cdesc->computeBB();
        if (omode == CDcallNone && isElectrical())
            cdesc->prptyAddStruct();
    }
    else {
        // Parent sdesc now has bad BB.
        reflectBadBB();
        cdesc->setBadBB();
    }

    // The following is called in lieu of insert().  Unless omode is
    // OpenModeNone, the cdesc is not added to the database here,
    // since its size may not be known yet.  Actual insertion is done
    // in reinsert().

    cdesc->linkIntoMaster();
    if (omode == CDcallNone && !db_insert(cdesc)) {
        delete cdesc;
        return (OIerror);
    }

    sBB.add(&cdesc->oBB());
    if (isElectrical()) {
        unsetConnected();
        CDs *sd = CDcdb()->findCell(cellname(), Physical);
        if (sd)
            sd->reflectBadAssoc();
    }
    else {
        reflectBadExtract();
        reflectBadGroundPlane();
    }
    if (pointer)
        *pointer = cdesc;
    return (OIok);
}


// Insert the object into the database.
//
bool
CDs::insert(CDo *odesc)
{
    if (!odesc)
        return (true);
    if (isImmutable()) {
        Errs()->add_error("attempt to add to immutable cell.");
        return (false);
    }
    if (odesc->ldesc()->isImmutable()) {
        Errs()->add_error("attempt to add to immutable layer.");
        return (false);
    }
    if (odesc->is_copy()) {
        Errs()->add_error("insert internal error, copy flag set.");
        return (false);
    }
    if (isSymbolic()) {
        if (odesc->type() == CDINSTANCE) {
            Errs()->add_error(
                "Attempt to add instance to symbolic representation.");
            return (false);
        }
        if (odesc->state() == CDInternal) {
            Errs()->add_error(
                "Attempt to add internal object to symbolic representation.");
            return (false);
        }
    }
    if (odesc->type() == CDINSTANCE) {
        if (odesc->ldesc() != CellLayer()) {
            Errs()->add_error(
                "Attempt to add instance to other than layer 0.");
            return (false);
        }
        CDc *cdesc = (CDc*)odesc;
        CDm *mdesc = cdesc->master();
        if (mdesc) {
            cdesc->linkIntoMaster();
            // relink master desc if not linked
            if (!mdesc->parent()) {
                CDm *m = linkMaster(mdesc);
                if (m != mdesc) {
                    // hmmm, master of same name exists
                    cdesc->setMaster(m);
                    cdesc->linkIntoMaster();
                    fprintf(stderr,
                        "Warning: master table inconsistency, repaired.\n");
                }
            }
        }
        else {
            Errs()->add_error("Instance with no master desc.");
            return (false);
        }

        // When reading an archive file, avoid insert/reinsert of
        // instances, this can be expensive.
        if (CD()->IsDeferInst() && (mdesc->isNullCelldesc() ||
                !mdesc->celldesc()->isBBvalid())) {
            sBB.addc(&odesc->oBB());
            return (true);
        }
    }
    else if (odesc->ldesc() == CellLayer()) {
        Errs()->add_error("Attempt to add non-instance to layer 0.");
        return (false);
    }

    if (!db_insert(odesc)) {
        Errs()->add_error("Internal: database insertion failed.");
        return (false);
    }

    sBB.addc(&odesc->oBB());
    CD()->ifObjectInserted(this, odesc);
    return (true);
}


// Public function to remove, then reinsert an object or instance,
// possibly with a bounding box change.  This should be used to change
// the bounding box of an object in the database.
//
bool
CDs::reinsert(CDo *odesc, const BBox *nBB)
{
    if (!odesc)
        return (false);
    CDtree *l = db_find_layer_head(odesc->ldesc());
    if (l) {
        if (l->is_deferred()) {
            // The odesc may or may not already be in the database. 
            // If it is already linked, the insert call will fail
            // harmlessly.

            if (nBB && odesc->oBB() != *nBB)
                odesc->set_oBB(*nBB);
            if (l->insert(odesc))
                CD()->ifObjectInserted(this, odesc);
            return (true);
        }
        if (odesc->in_db()) {
            if (!l->remove(odesc))
                return (false);
        }
        if (nBB && odesc->oBB() != *nBB) {
            odesc->set_oBB(*nBB);
            setBBvalid(false);
        }
        if (l->insert(odesc)) {
            CD()->ifObjectInserted(this, odesc);
            return (true);
        }
        return (false);
    }
    if (nBB && odesc->oBB() != *nBB) {
        odesc->set_oBB(*nBB);
        setBBvalid(false);
    }
    if (db_insert(odesc)) {
        CD()->ifObjectInserted(this, odesc);
        return (true);
    }
    return (false);
}


CDm *
CDs::insertMaster(CDcellName mname)
{
    if (isSymbolic()) {
        Errs()->add_error(
            "Attempt to insert master table to symbolic representation.");
        return (0);
    }
    CDm *m = findMaster(mname);
    if (!m) {
        m = new CDm(mname);
        m->setParent(this);
        if (sMasters & 1) {
            itable_t<CDm> *t = (itable_t<CDm>*)(sMasters & ~1);
            t->link(m, false);
            t = t->check_rehash();
            sMasters = (unsigned long)t | 1;
        }
        else {
            int cnt = 0;
            for (CDm *mm = (CDm*)sMasters; mm; mm = mm->tab_next())
                cnt++;
            if (cnt < ST_MAX_DENS) {
                m->set_tab_next((CDm*)sMasters);
                sMasters = (unsigned long)m;
            }
            else {
                itable_t<CDm> *t = new itable_t<CDm>;
                CDm *mm = (CDm*)sMasters;
                while (mm) {
                    CDm *mx = mm;
                    mm = mm->tab_next();
                    mx->set_tab_next(0);
                    t->link(mx, false);
                }
                t->link(m, false);
                t = t->check_rehash();
                sMasters = (unsigned long)t | 1;
            }
        }
    }
    return (m);
}


CDm *
CDs::removeMaster(CDcellName mname)
{
    if (!mname)
        return (0);
    CDm *m = 0;
    if (sMasters & 1)
        m = ((itable_t<CDm>*)(sMasters & ~1))->remove(mname->string());
    else {
        CDm *mp = 0;
        for (CDm *mm = (CDm*)sMasters; mm; mm = mm->tab_next()) {
            if (mm->cellname() == mname) {
                if (!mp)
                    sMasters = (unsigned long)mm->tab_next();
                else
                    mp->set_tab_next(mm->tab_next());
                m = mm;
                m->set_tab_next(0);
                break;
            }
            mp = mm;
        }
    }
    if (m)
        m->setParent(0);
    return (m);
}


CDm *
CDs::linkMaster(CDm *md)
{
    if (isSymbolic()) {
        Errs()->add_error(
            "Attempt to link master table to symbolic representation.");
        return (0);
    }
    CDm *m = 0;
    if (sMasters & 1) {
        itable_t<CDm> *t = (itable_t<CDm>*)(sMasters & ~1);
        m = t->link(md);
        t = t->check_rehash();
        sMasters = (unsigned long)t | 1;
    }
    else {
        int cnt = 0;
        for (CDm *mm = (CDm*)sMasters; mm; mm = mm->tab_next()) {
            if (md->cellname() == mm->cellname()) {
                mm->setParent(this);
                return (mm);
            }
            cnt++;
        }
        if (cnt < ST_MAX_DENS) {
            md->set_tab_next((CDm*)sMasters);
            sMasters = (unsigned long)md;
            m = md;
        }
        else {
            itable_t<CDm> *t = new itable_t<CDm>;
            CDm *mm = (CDm*)sMasters;
            while (mm) {
                CDm *mx = mm;
                mm = mm->tab_next();
                mx->set_tab_next(0);
                t->link(mx, false);
            }
            m = t->link(md);
            t = t->check_rehash();
            sMasters = (unsigned long)t | 1;
        }
    }
    if (m)
        m->setParent(this);
    return (m);
}


CDm *
CDs::unlinkMaster(CDm *md)
{
    CDm *m = 0;
    if (sMasters & 1)
        m = ((itable_t<CDm>*)(sMasters & ~1))->unlink(md);
    else {
        CDm *mp = 0;
        for (CDm *mm = (CDm*)sMasters; mm; mm = mm->tab_next()) {
            if (mm == md) {
                if (!mp)
                    sMasters = (unsigned long)mm->tab_next();
                else
                    mp->set_tab_next(mm->tab_next());
                m = mm;
                m->set_tab_next(0);
                break;
            }
            mp = mm;
        }
    }
    if (m)
        m->setParent(0);
    return (m);
}


void
CDs::linkMasterRef(CDm *md)
{
    CDs *sd = owner();
    if (sd) {
        sd->linkMasterRef(md);
        return;
    }

    if (sMasterRefs & 1) {
        ptable_t<CDm> *t = (ptable_t<CDm>*)(sMasterRefs & ~1);
        t->add(md);
        t = t->check_rehash();
        sMasterRefs = (unsigned long)t | 1;
    }
    else {
        int cnt = 0;
        for (CDm *mm = (CDm*)sMasterRefs; mm; mm = mm->ptab_next())
            cnt++;
        if (cnt < ST_MAX_DENS) {
            md->set_ptab_next((CDm*)sMasterRefs);
            sMasterRefs = (unsigned long)md;
        }
        else {
            ptable_t<CDm> *t = new ptable_t<CDm>;
            CDm *mm = (CDm*)sMasterRefs;
            while (mm) {
                CDm *mx = mm;
                mm = mm->ptab_next();
                mx->set_ptab_next(0);
                t->add(mx);
            }
            t->add(md);
            t = t->check_rehash();
            sMasterRefs = (unsigned long)t | 1;
        }
    }
}


void
CDs::unlinkMasterRef(CDm *md)
{
    CDs *sd = owner();
    if (sd) {
        sd->unlinkMasterRef(md);
        return;
    }

    if (sMasterRefs & 1)
        ((ptable_t<CDm>*)(sMasterRefs & ~1))->remove(md);
    else {
        CDm *mp = 0;
        for (CDm *mm = (CDm*)sMasterRefs; mm; mm = mm->ptab_next()) {
            if (mm == md) {
                if (!mp)
                    sMasterRefs = (unsigned long)mm->ptab_next();
                else
                    mp->set_ptab_next(mm->ptab_next());
                md->set_ptab_next(0);
                break;
            }
            mp = mm;
        }
    }
}


// Update the names of instance terminals.
//
void
CDs::updateTermNames(CDs::UTNtype t)
{
    bool cache_state = CD()->EnableNameCache(true);
    CDm_gen mgen(this, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;
        if (!mdesc->hasInstances())
            continue;
        if (t == UTNinstances) {
            // update subciruits and devices
            if (isDevOrSubc(msdesc->elecCellType())) {
                CDc_gen cgen(mdesc);
                for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
                    c->updateTermNames();
            }
        }
        else if (t == UTNterms) {
            // update terminals
            CDelecCellType et = msdesc->elecCellType();
            if (et == CDelecTerm) {
                CDc_gen cgen(mdesc);
                for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
                    c->updateTermNames();
            }
        }
    }
    CD()->EnableNameCache(cache_state);
}


// Reflect the terminal names to all instances.
//
void
CDs::reflectTermNames()
{
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        CDc_gen cgen(m);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
            c->updateTermNames();
    }
}


// Reflect terminal locations of electrical cell to all instances. 
// This will also check the index numbers, and reassign if necessary
// to make the numbering compact.
//
void
CDs::reflectTerminals()
{
    if (!isElectrical() || !prpty(P_NODE))
        return;

    CDs *sd = owner();
    if (sd) {
        sd->reflectTerminals();
        return;
    }

    // Filter out node properties that have duplicate or too-large
    // index numbers, then renumber them compactly.  The map array
    // maps new index values into old.
    //
    // This will also check the bus terminals.  The index field is the
    // node index of the connector range start.  In order to
    // facilitate easy cell/instance property correspondence, the
    // index is required to be unique.  We enforce that here (as for
    // nodes), but otherwise leave the index alone.

    int *map = 0;
    CDp_snode **nodeary;
    checkBterms();
    unsigned int asize = checkTerminals(&nodeary);
    if (asize) {
        map = new int[asize];
        memset(map, 0, asize*sizeof(int));

        int cnt = 0;
        for (unsigned int i = 0; i < asize; i++) {
            if (!nodeary[i])
                continue;
            map[cnt] = i;
            nodeary[i]->set_index(cnt);
            cnt++;
        }
        unsetConnected();
        delete [] nodeary;

        // Update the bterm indices.
        CDp_bsnode *pb = (CDp_bsnode*)prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            for (int i = 0; i < cnt; i++) {
                if (map[i] == (int)pb->index()) {
                    pb->set_index(i);
                    break;
                }
            }
        }
    }

    // Now reflect to all instances.
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        if (!m->parent())
            continue;
        CDc_gen cgen(m);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next())
            cdesc->updateTerminals(map);
        m->parent()->unsetConnected();
        CDcbin cbin(m->parent());
        if (cbin.phys())
            cbin.phys()->unsetConnected();
    }
    delete [] map;
}


// Node property consistency test.  Pull out the properties, delete
// properties with duplicate or crazy indices.  Put the properties
// back, in order.  This does NOT renumber, call reflectTerminals to
// compactly renumber the nodes.
//
// This will return a node array if the address is given.
//
unsigned int
CDs::checkTerminals(CDp_snode ***nary)
{
    if (nary)
        *nary = 0;
    if (!isElectrical())
        return (0);

    unsigned int nsize = 0;
    CDp_snode *pn = (CDp_snode*)prpty(P_NODE);
    if (!pn)
        return (0);
    for ( ; pn; pn = pn->next()) {
        if (pn->index() > P_NODE_MAX_INDEX)
            continue;
        if (pn->index() > nsize)
            nsize = pn->index();
    }
    nsize++;

    CDp_snode **node_ary = new CDp_snode*[nsize];
    memset(node_ary, 0, nsize*sizeof(CDp_snode*));

    while ((pn = (CDp_snode*)prpty(P_NODE)) != 0) {
        prptyUnlink(pn);
        if (pn->index() > P_NODE_MAX_INDEX) {
            delete pn;
            continue;
        }
        if (!node_ary[pn->index()])
            node_ary[pn->index()] = pn;
        else {
            // bad index
            CD()->ifPrintCvLog(IFLOG_WARN,
                "Deleting node property %s in %s, index %d was a duplicate.",
                pn->term_name(), cellname()->string(), pn->index());
            delete pn;
        }
    }

    // Put 'em back.
    for (int i = nsize - 1; i >= 0; i--) {
        if (!node_ary[i])
            continue;
        node_ary[i]->set_next_prp(prptyList());
        setPrptyList(node_ary[i]);
    }
    if (nary)
        *nary = node_ary;
    else
        delete [] node_ary;
    return (nsize);
}


// Check the bus terminals.  The index field is the node index of the
// connector range start.  In order to facilitate easy cell/instance
// property correspondence, the index is required to be unique.  We
// enforce that here, but otherwise leave the index alone.
//
void
CDs::checkBterms()
{
    unsigned int count = 0;
    unsigned int maxn = 0;
    CDp_bsnode *pb = (CDp_bsnode*)prpty(P_BNODE);
    for ( ; pb; pb = pb->next()) {
        if (pb->index() <= P_NODE_MAX_INDEX) {
            if (maxn < pb->index())
                maxn = pb->index();
        }
        count++;
    }

    // Check the bus terminals for duplicate index values.
    if (count) {
        maxn++;
        CDp_bsnode **btary = new CDp_bsnode*[maxn];
        memset(btary, 0, maxn*sizeof(CDp_bsnode*));
        while ((pb = (CDp_bsnode*)prpty(P_BNODE)) != 0) {
            prptyUnlink(pb);
            if (pb->index() <= P_NODE_MAX_INDEX && !btary[pb->index()])
                btary[pb->index()] = pb;
            else {
                // error, duplicate or bad index
                CD()->ifPrintCvLog(IFLOG_WARN,
                    "Deleting bus node property %s in %s, index %d was "
                    "a duplicate.",
                    pb->term_name(), cellname()->string(), pb->index());
                delete pb;
            }
        }
        for (unsigned int i = 0; i < maxn; i++) {
            if (btary[i]) {
                btary[i]->set_next_prp(sPrptyList);
                sPrptyList = btary[i];
            }
        }
        delete [] btary;
    }
}


// Remove object from database.  If save is true, simply unlink the
// object, but don't free it.  Return false if the object can't be
// deleted.
//
// Note that there is no error message on failure, the caller should
// pre-check for immutability, etc. when necessary.
//
bool
CDs::unlink(CDo *odesc, int save)
{
    if (!odesc)
        return (true);
    if (isImmutable())
        return (false);
    if (odesc->ldesc()->isImmutable())
        return (false);
    if (odesc->is_copy())
        return (false);

    if (isSymbolic()) {
        // Just ignore.
        if (odesc->state() == CDInternal)
            return (true);
    }

    odesc->set_state(CDDeleted);
    if (isElectrical() &&
            (odesc->type() == CDWIRE || odesc->type() == CDINSTANCE))
        hyDeleteReference(odesc, true);

    if (!isElectrical() && odesc->type() != CDLABEL)
        odesc->prptyClearInternal();

    CD()->ifInvalidateObject(this, odesc, save);

    db_remove(odesc);
    if (odesc->type() == CDINSTANCE)
        // Put in unlinked list
        // The master desc remains linked in parent until all unlinked
        // objects are deleted, to avoid creating multiple master descs
        // for the same cell.
        ((CDc*)odesc)->unlinkFromMaster(true);

    // This removes any mention of odesc from the cell and other objects
    // still in the database
    prptyPurge(odesc);

    // check if valid BB
    if (!(sBB > odesc->oBB()))
        setBBvalid(false);

    if (!save)
        delete odesc;
    return (true);
}


// Return the object descriptor of the device named in name.  This
// applies to electrical cells.
//
CDc*
CDs::findInstance(const char *dname)
{
    if (!isElectrical())
        return (0);
    CDm_gen mgen(this, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc || !msdesc->isDevice())
            continue;
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (!cdesc->is_normal())
                continue;
            CDp_name *pn = (CDp_name*)cdesc->prpty(P_NAME);
            if (pn) {
                const char *instname = cdesc->getBaseName(pn);
                if (!strcmp(dname, instname))
                    return (cdesc);
            }
        }
    }
    return (0);
}


namespace {
    // Create an instance of targ with upper left corner at xpos,
    // ypos.  The xpos is increased beyond the new cell width.
    //
    void
    addinst(CDs *sdesc, CDs *targ, int *xpos, int *ypos)
    {
        CallDesc calldesc;
        calldesc.setCelldesc(targ);
        calldesc.setName(targ->cellname());

        const BBox *sBB = targ->BB();
        int x = sBB->left;
        int y = sBB->top;
        int resol = sdesc->isElectrical() ? CDphysResolution : CDelecResolution;
        x = ((x + resol/2)/resol)*resol;
        y = ((y + resol/2)/resol)*resol;

        CDtx tx(false, 1, 0, *xpos - x, *ypos - y, 1.0);

        CDc *cdesc;
        if (OIfailed(sdesc->makeCall(&calldesc, &tx, 0, CDcallNone, &cdesc)))
            return;
        CD()->ifRecordObjectChange(sdesc, 0, cdesc);
        CD()->ifLabelInstance(sdesc, cdesc);
        *xpos += cdesc->oBB().width() + 5*resol;
    }
}


// Add the corresponding instances into the counterpart of this.  I.e.,
// if this is electrical, add the physical instances of subcircuits in
// this, if they are not empty and do not exist.  The new cells are
// arrayed along the bottom of the existing boundary.
//
void
CDs::addMissingInstances()
{
    if (isSymbolic())
        return;

    // find complement cell
    CDs *dual;
    if (isElectrical())
        dual = CDcdb()->findCell(cellname(), Physical);
    else
        dual = CDcdb()->findCell(cellname(), Electrical);
    if (!dual)
        return;

    // set location for new subcells
    int xpos, ypos;
    if (isEmpty())
        xpos = ypos = 0;
    else {
        int resol = isElectrical() ? CDphysResolution : CDelecResolution;
        xpos = (sBB.left/resol)*resol;
        ypos = (sBB.bottom/resol)*resol - 2*resol;
    }

    CDm_gen mgen(dual, GEN_MASTERS);
    for (CDm* md_d = mgen.m_first(); md_d; md_d = mgen.m_next()) {
        CDs *msdesc = md_d->celldesc();
        if (!msdesc)
            continue;
        CDs *altdesc;
        if (msdesc->isElectrical()) {
            if (msdesc->isDevice())
                continue;
            altdesc = CDcdb()->findCell(msdesc->cellname(), Physical);
            if (!altdesc)
                continue;
        }
        else {
            altdesc = CDcdb()->findCell(msdesc->cellname(), Electrical);
            if (!altdesc)
                continue;
            if (!altdesc->prpty(P_NODE))
                continue;
        }
        if (altdesc->isEmpty())
            continue;
        int dcnt = 0;
        CDc_gen cgen_d(md_d);
        for (CDc *c = cgen_d.c_first(); c; c = cgen_d.c_next())
            dcnt++;

        CDm *md = findMaster(md_d->cellname());
        int cnt = 0;
        if (md) {
            CDc_gen cgen(md);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
                cnt++;
        }
        while (cnt < dcnt) {
            addinst(this, altdesc, &xpos, &ypos);
            cnt++;
        }
    }
}


// Return true if the cell contains subcell instances.
//
bool
CDs::hasSubcells() const
{
    CDm_gen mgen(this, GEN_MASTERS);
    for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
        if (m->hasInstances())
            return (true);
    }
    return (false);
}


// Return instance count, and list of instances if list is not nil.
//
int
CDs::listSubcells(stringnumlist **list, bool expand) const
{
    int count = 0;
    CDm_gen mgen(this, GEN_MASTERS);
    for (CDm *m = mgen.m_first(); m; m = mgen.m_next())
        count += m->listInstance(list, expand);
    return (count);
}


namespace {
    struct sHier : public cTfmStack
    {
        sHier(int dp, const BBox  *AOI)
            {
                h_ctab = new SymTab(false, false);
                h_depth = dp >= 0 ? dp : CDMAXCALLDEPTH;
                h_AOI = *AOI;
            }

        ~sHier()
            {
                delete h_ctab;
            }

        void setup_cells(const CDs*, int = 0);
        stringlist *get_list(bool, const CDs*);

    private:
        SymTab *h_ctab;
        BBox h_AOI;
        int h_depth;
    };


    void
    sHier::setup_cells(const CDs *sdesc, int dp)
    {
        CDm_gen mgen(sdesc, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            CDs *msdesc = mdesc->celldesc();
            if (!msdesc || msdesc->isDevice())
                continue;
            CDc_gen cgen(mdesc);
            for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
                if (!cdesc->is_normal())
                    continue;

                TPush();
                unsigned int x1, x2, y1, y2;
                if (TOverlapInst(cdesc, &h_AOI, &x1, &x2, &y1, &y2)) {
                    CDap ap(cdesc);
                    int tx, ty;
                    TGetTrans(&tx, &ty);  
                    xyg_t xyg(x1, x2, y1, y2);
                    do {
                        TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);  
                        // Box test instance array element with AOI.
                        BBox BB = *msdesc->BBforInst(cdesc);
                        TBB(&BB, 0);  
                        if (BB.intersect(&h_AOI, false)) {
                            if (h_ctab->get((unsigned long)msdesc) == ST_NIL)
                                h_ctab->add((unsigned long)msdesc, 0, false);
                            if (dp < h_depth)
                                setup_cells(msdesc, dp+1);
                        }
                        TSetTrans(tx, ty);
                    } while (xyg.advance());
                }
                TPop();  
            }
        }
    }


    // Sort comparison, strip junk prepended to cell names.
    //
    bool
    cl_comp(const char *s1, const char *s2)
    {
        while (isspace(*s1) || *s1 == '*' || *s1 == '+')
            s1++;
        while (isspace(*s2) || *s2 == '*' || *s2 == '+')
            s2++;
        return (strcmp(s1, s2) < 0);
    }


    stringlist *
    sHier::get_list(bool mark, const CDs *sdesc)
    {
        stringlist *s0 = 0;
        if (sdesc) {
            if (mark) {
                char *s = new char[strlen(sdesc->cellname()->string()) + 3];
                int j = 0;
                if (sdesc->isModified())
                    s[j++] = '+';
                if (!sdesc->isSubcell())
                    s[j++] = '*';
                if (!j)
                    s[j++] = ' ';
                strcpy(s+j, sdesc->cellname()->string());
                s0 = new stringlist(s, s0);
            }
            else {
                s0 = new stringlist(lstring::copy(
                    sdesc->cellname()->string()), s0);
            }
        }
        SymTabGen gen(h_ctab, false);
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            CDs *sd = (CDs*)ent->stTag;
            if (mark) {
                char *s = new char[strlen(sd->cellname()->string()) + 2];
                int j = 1;
                if (sd->isModified()) {
                    *s = '+';
                    *(s+1) = '*';
                    j = 2;
                }
                else
                    *s = ' ';
                strcpy(s+j, sd->cellname()->string());
                s0 = new stringlist(s, s0);
            }
            else {
                s0 = new stringlist(lstring::copy(
                    sd->cellname()->string()), s0);
            }
        }
        s0->sort(mark ? &cl_comp : 0);
        return (s0);
    }
}


// Return a sorted list of cells used in the hierarchy to depth, whose
// instances are displayed in AOI (if given).  If incl_top, include
// the top cell (if it overlaps AOI).  If mark, add a '+' prefix to
// the names of modified cells.
//
stringlist *
CDs::listSubcells(int depth, bool incl_top, bool mark, const BBox *AOI)
{
    stringlist *s0 = 0;
    if (!AOI) {
        if (incl_top) {
            if (mark) {
                char *s = new char[strlen(cellname()->string()) + 3];
                int j = 0;
                if (isModified())
                    s[j++] = '+';
                if (!isSubcell())
                    s[j++] = '*';
                if (!j)
                    s[j++] = ' ';
                strcpy(s+j, cellname()->string());
                s0 = new stringlist(s,  s0);
            }
            else
                s0 = new stringlist(lstring::copy(cellname()->string()), s0);
        }

        if (depth < 0)
            depth = CDMAXCALLDEPTH + 1;
        CDgenHierDn_s gen(this, depth);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0) {
            if (sd->isDevice())
                continue;
            if (mark) {
                char *s = new char[strlen(sd->cellname()->string()) + 2];
                if (sd->isModified())
                    *s = '+';
                else
                    *s = ' ';
                strcpy(s+1, sd->cellname()->string());
                s0 = new stringlist(s,  s0);
            }
            else {
                s0 = new stringlist(lstring::copy(
                    sd->cellname()->string()), s0);
            }
        }
        if (err) {
            s0->free();
            s0 = 0;
        }
        s0->sort(mark ? &cl_comp : 0);
        return (s0);
    }
    if (AOI->intersect(&sBB, true)) {
        sHier hier(depth, AOI);
        hier.setup_cells(this);
        return (hier.get_list(mark, this));
    }
    return (0);
}


// Return instantiation count, and list of parents if list is not nil.
//
int
CDs::listParents(stringnumlist **list, bool expand) const
{
    int count = 0;
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        count += m->listInstance(0, expand);
        if (list) {
            CDs *parent = m->parent();
            if (parent)
                *list = new stringnumlist(
                    lstring::copy(parent->cellname()->string()), count, *list);
        }
    }
    return (count);
}


// Function returns true if an object on ld exists in the hierarchy.
//
bool
CDs::hasLayer(const CDl *ld)
{
    if (ld) {
        CDgenHierDn_s gen(this);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0) {
            if (!sd->db_is_empty(ld))
                return (true);
        }
    }
    return (false);
}


// Print diagnostic for layer in database.
//
void
CDs::bincnt(const CDl *ld, int lev) const
{
    if (ld) {
        printf("Cell %s Layer %s\n", cellname()->string(), ld->name());
        db_bincnt(ld, lev);
    }
}


//-----------------------------------------------------------------------------
// Property functions.
//-----------------------------------------------------------------------------

// Return the type of the cell for electrical mode.
//
CDelecCellType
CDs::elecCellType(CDpfxName *nret)
{
    if (nret)
        *nret = 0;
    if (!isElectrical())
        return (CDelecBad);

    CDelecCellType etype = CDelecBad;
    CDp_name *pa = (CDp_name*)prpty(P_NAME);
    if (!pa) {
        // Hmmm, no name property.  If the cell contains devices or
        // subcells, we will add a name property, making this a
        // subcell.
        bool needX = false;
        CDm_gen mgen(this, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            if (!mdesc->hasInstances())
                continue;
            CDs *sd = mdesc->celldesc();
            if (!sd)
                continue;
            CDelecCellType et = sd->elecCellType();
            if (et == CDelecDev || et == CDelecSubc || et == CDelecMacro) {
                needX = true;
                break;
            }
        }
        if (needX) {
            char buf[64];
            sprintf(buf, "X 0 %s", cellname()->string());
            prptyAdd(P_NAME, buf);
            etype = CDelecSubc;
            if (nret) {
                pa = (CDp_name*)prpty(P_NAME);
                *nret = pa->name_string();
            }
        }
        else {
            // Otherwise, a cell without a name property and no
            // subcells and with exactly one node property is a "gnd"
            // device.  Otherwise this is a "null" device.

            CDp_node *pn = (CDp_node*)prpty(P_NODE);
            if (pn && !pn->next() && !sMasters)
                etype = CDelecGnd;
            else
                etype = CDelecNull;
        }
    }
    else {
        if (!pa->name_string()) {
            // "can't happen"
            char bf[4];
            bf[0] = P_NAME_NULL;
            bf[1] = 0;
            pa->set_name_string(bf);
        }
        int key = *pa->name_string()->string();
        if (nret)
            *nret = pa->name_string();

        if (pa->is_subckt()) {
            // If set, this is a subcircuit, not from the device
            // library.  The Subckt flag should be set, and the key
            // should be the SPICE subcircuit invocation character
            // 'x'.

            if (key == 'x' || key == 'X')
                etype = CDelecSubc;
            // Otherwise an error.
        }
        else if (key == 0 || key == P_NAME_NULL) {
            // A "null" device is for decoration only, it is not
            // electrically active, and should not have node
            // properties.

            etype = CDelecNull;
        }
        else if (key == P_NAME_TERM) {
            // A scalar terminal.

            etype = CDelecTerm;
        }
        else if (key == P_NAME_BTERM_DEPREC) {
            // Obsolete prefix, update.

            char tbf[2];
            tbf[0] = P_NAME_TERM;
            tbf[1] = 0;
            pa->set_name_string(tbf);
            etype = CDelecTerm;
        }
        else if (isalpha(key)) {
            // A regular device, the key is the conventional SPICE key
            // character.

            // If a device is keyed by 'X', it is really a subcircuit
            // macro call to the model library.  The subname field
            // must be 0 for devices in the device library file.  The
            // name of the subcircuit macro is stored in a model
            // property, so that the text will be added along with the
            // model text.  The subckt macros are saved in the model
            // database.

            if (key == 'x' || key == 'X')
                etype = CDelecMacro;
            else
                etype = CDelecDev;
        }
        else {
            // Name not recognized, null device.

            etype = CDelecNull;
        }
    }
    return (etype);
}


CDp *
CDs::prptyList() const
{
    CDs *sd = owner();
    if (sd)
        return (sd->prptyList());
    return (sPrptyList);
}


void
CDs::setPrptyList(CDp *pd)
{
    CDs *sd = owner();
    if (sd) {
        sd->setPrptyList(pd);
        return;
    }
    sPrptyList = pd;
}


// This is the correct way to copy cell properties.  It makes sure that
// the P_SYMBLC property is handled correctly, i.e., that the owner
// field in the rep is set.
//
CDp *
CDs::prptyCopy(const CDp *pdesc)
{
    if (isElectrical() && pdesc->value() == P_SYMBLC) 
        return (((CDp_sym*)pdesc)->dup(this));
    return (pdesc->dup());
}


CDp *
CDs::prpty(int pnum) const
{
    CDs *sd = owner();
    if (sd)
        return (sd->prpty(pnum));
    for (CDp *pd = sPrptyList; pd; pd = pd->next_prp())
        if (pd->value() == pnum)
            return (pd);
    return (0);
}


// This is called to apply properties read from a file to the cell or
// object just read from the file.  If inappropriate properties are
// found, they are removed from the property list and freed.  The
// remainder of the list is *not* freed.  A list of warning messages
// is returned.
//
stringlist *
CDs::prptyApplyList(CDo *odesc, CDp **pplist)
{
    CDs *sd = owner();
    if (sd)
        return (sd->prptyApplyList(odesc, pplist));

    const char *msg1 = "reserved property number %d (ignored)";
    const char *msg2 = "bad property string for value %d (ignored)";
    stringlist *s0 = 0;

    // If these properties are found, then they came from the super-
    // master and are therefor known-good.  They override any property
    // read from the file.
    //
    int n1 = 0;
    int n2 = 0;
    if (odesc && odesc->type() == CDINSTANCE && displayMode() == Physical) {
        for (CDp *p = odesc->prpty_list(); p; p = p->next_prp()) {
            if (p->value() == XICP_PC)
                n1++;
            if (p->value() == XICP_PC_PARAMS)
                n2++;
        }
    }

    CDp *pp = 0, *pn;
    for (CDp *pd = *pplist; pd; pd = pn) {
        pn = pd->next_prp();
        if (prpty_reserved(pd->value()) || prpty_pseudo(pd->value())) {
            char *tbuf = new char[strlen(msg1) + 16];
            sprintf(tbuf, msg1, pd->value());
            s0 = new stringlist(tbuf, s0);
            if (!pp)
                *pplist = pn;
            else
                pp->set_next_prp(pn);
            delete pd;
            continue;
        }
        if (pd->value() == XICP_PC) {
            if (n1) {
                if (!pp)
                    *pplist = pn;
                else
                    pp->set_next_prp(pn);
                delete pd;
                continue;
            }
            n1++;
        }
        if (pd->value() == XICP_PC_PARAMS) {
            if (n2) {
                if (!pp)
                    *pplist = pn;
                else
                    pp->set_next_prp(pn);
                delete pd;
                continue;
            }
            n2++;
        }
        bool ret = odesc ?
            odesc->prptyAdd(pd->value(), pd->string(), displayMode()) :
            prptyAdd(pd->value(), pd->string());
        if (!ret) {
            char *tbuf = new char[strlen(msg2) + 16];
            sprintf(tbuf, msg2, pd->value());
            s0 = new stringlist(tbuf, s0);
            if (Errs()->has_error()) {
                char *tt = lstring::copy(Errs()->get_error());
                s0 = new stringlist(tt, s0);
            }
            if (!pp)
                *pplist = pn;
            else
                pp->set_next_prp(pn);
            delete pd;
            continue;
        }
        pp = pd;
    }
    // Historically, prptyInstPatch and prptyLabelPatch were called
    // from here.  They are now called in prptyPatchAll, which is
    // called after all instances and geometry have been read,

    return (s0);
}


// This must be called when reading in a new electrical cell, after
// all instances and objects have been read.
//
// Go through the instances and call prptyInstPatch.  Similarly, call
// prptyLabelPatch for all labels.  Previously, these calls would be
// made while reading, which required that the instances be in memory
// before labels.  This was manageable, but the new wire labels broke
// that, so now these calls are deferred until all reading of a cell
// is complete.
//
void
CDs::prptyPatchAll()
{
    if (!isElectrical())
        return;

    // First the instances.  The instances may not be in the spatial
    // database yet, so use the list generators.

    CDm_gen mgen(this, GEN_MASTERS);
    CDm *mdesc;
    while ((mdesc = mgen.next()) != 0) {
        CDc_gen cgen(mdesc);
        CDc *cdesc;
        while ((cdesc = cgen.next()) != 0)
            prptyInstPatch(cdesc);
    }

    // Now the labels.  These will be in the spatial database.

    CDlgen gen(Electrical);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(this, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (odesc->type() != CDLABEL)
                continue;
            prptyLabelPatch((CDla*)odesc);
        }
    }
}


// Update cell flags from the string.  The string can be in one of two
// forms:  a hex number, or a space-separated list of tokens.  This is
// used to interpret the XICP_FLAGS property.
//
void
CDs::prptyUpdateFlags(const char *string)
{
    if (!string)
        return;

    CDs *sd = owner();
    if (sd) {
        sd->prptyUpdateFlags(string);
        return;
    }

    int flg = 0;
    if (sscanf(string, "%x", &flg) == 1) {
        setOpaque(flg & 0x1);
        setConnector(flg & 0x2);
        setUflag(0, flg & 0x4);
        setUflag(1, flg & 0x8);
        setUflag(2, flg & 0x10);
    }
    else {
        setOpaque(false);
        setConnector(false);
        setUflag(0, false);
        setUflag(1, false);
        setUflag(2, false);
        char *tok;
        while ((tok = lstring::gettok(&string)) != 0) {
            if (lstring::cieq(tok, "OPAQUE")) {
                setOpaque(true);
                setExtracted(false);
            }
            else if (lstring::cieq(tok, "CONNECTOR")) {
                setConnector(true);
                setExtracted(false);
            }
            else if (lstring::cieq(tok, "USER0"))
                setUflag(0, true);
            else if (lstring::cieq(tok, "USER1"))
                setUflag(1, true);
            delete [] tok;
        }
    }
}


// Add a property to CDs, using the given value and string.
//
bool
CDs::prptyAdd(int value, const char *string)
{
    if (string == 0)
        return (true);

    CDs *sd = owner();
    if (sd)
        return (sd->prptyAdd(value, string));

    CDp *pdesc;
    if (isElectrical()) {
        switch (value) {
        case P_MODEL:
        case P_VALUE:
        case P_PARAM:
        case P_OTHER:
            pdesc = new CDp_user(this, string, value);
            break;
        case P_NOPHYS:
            {
                const char *pstr = "nophys";
                if (string && (*string == 's' || *string == 'S'))
                    pstr = "shorted";
                pdesc = new CDp(pstr, value);
            }
            break;
        case P_VIRTUAL:
            pdesc = new CDp("virtual", value);
            break;
        case P_FLATTEN:
            pdesc = new CDp("flatten", value);
            break;
        case P_RANGE:
            // No error, just don't do it.
            return (true);
        case P_BNODE:
            pdesc = new CDp_bsnode;
            if (!((CDp_bsnode*)pdesc)->parse_bsnode(string)) {
                delete pdesc;
                return (false);
            }
            break;
        case P_NODE:
            pdesc = new CDp_snode;
            if (!((CDp_snode*)pdesc)->parse_snode(this, string)) {
                delete pdesc;
                return (false);
            }
            break;
        case P_NAME:
            pdesc = new CDp_name;
            if (!((CDp_name*)pdesc)->parse_name(string)) {
                delete pdesc;
                return (false);
            }
            // Set the Device flag.
            setDevice(!((CDp_name*)pdesc)->is_subckt());
            break;
        case P_LABLOC:
            pdesc = new CDp_labloc;
            if (!((CDp_labloc*)pdesc)->parse_labloc(string)) {
                delete pdesc;
                return (false);
            }
            break;
        case P_MUT:
            pdesc = new CDp_mut;
            if (!((CDp_mut*)pdesc)->parse_mut(string)) {
                delete pdesc;
                return (false);
            }
            break;
        case P_NEWMUT:
            pdesc = new CDp_nmut;
            if (!((CDp_nmut*)pdesc)->parse_nmut(string)) {
                delete pdesc;
                return (false);
            }
            break;
        case P_BRANCH:
            pdesc = new CDp_branch;
            if (!((CDp_branch*)pdesc)->parse_branch(string)) {
                delete pdesc;
                return (false);
            }
            break;
        case P_LABRF:
        case P_MUTLRF:
            // No error, just don't do it.
            return (true);
        case P_SYMBLC:
            pdesc = new CDp_sym;
            if (!((CDp_sym*)pdesc)->parse_sym(this, string)) {
                delete pdesc;
                return (false);
            }
            break;
        case P_NODMAP:
            pdesc = new CDp_nodmp;
            if (!((CDp_nodmp*)pdesc)->parse_nodmp(string)) {
                delete pdesc;
                return (false);
            }
            break;
        case P_MACRO:
            pdesc = new CDp("macro", value);
            break;
        case P_DEVREF:
            // No error, just don't do it.
            return (true);
        default:
            pdesc = new CDp(string, value);
            break;
        }
    }
    else {
        pdesc = new CDp(string, value);
        if (pdesc->value() == XICP_FLAGS)
            prptyUpdateFlags(pdesc->string());
        else if (pdesc->value() == XICP_CHD_REF) {
            sChdPrp prp(pdesc->string());
            sBB = *prp.get_bb();
            setBBvalid(true);
            setChdRef(true);
            setImmutable(true);
        }
        else if (pdesc->value() == XICP_STDVIA)
            setViaSubMaster(true);
    }

    pdesc->set_next_prp(sPrptyList);
    sPrptyList = pdesc;
    return (true);
}


// Add a copy of pdesc to CDs, return pointer to copy.
//
CDp *
CDs::prptyAddCopy(CDp *pdesc)
{
    if (!pdesc)
        return (0);

    CDs *sd = owner();
    if (sd)
        return (sd->prptyAddCopy(pdesc));

    CDp *pcopy = prptyCopy(pdesc);
    if (pcopy) {
        pcopy->set_next_prp(sPrptyList);
        sPrptyList = pcopy;
    }
    return (pcopy);
}


// Add a copy of the entire prpty list in pdesc to CDs.
//
void
CDs::prptyAddCopyList(CDp *pdesc)
{
    if (!pdesc)
        return;

    CDs *sd = owner();
    if (sd) {
        sd->prptyAddCopyList(pdesc);
        return;
    }

    CDp *pp = 0, *p0 = 0;
    for ( ; pdesc; pdesc = pdesc->next_prp()) {
        if (prpty_reserved(pdesc->value()))
            continue;
        CDp *pcopy = prptyCopy(pdesc);
        if (!pcopy)
            continue;
        if (!pp)
            pp = p0 = pcopy;
        else {
            pp->set_next_prp(pcopy);
            pp = pp->next_prp();
        }
        pcopy->set_next_prp(0);
    }
    if (p0) {
        pp->set_next_prp(sPrptyList);
        sPrptyList = p0;
    }
}


// Unlink the CDp descriptor desc from the CDs property list.
//
CDp *
CDs::prptyUnlink(CDp *pdesc)
{
    if (!pdesc)
        return (0);

    CDs *sd = owner();
    if (sd)
        return (sd->prptyUnlink(pdesc));

    CDp *pt1 = 0;
    for (CDp *pd = sPrptyList; pd; pt1 = pd, pd = pd->next_prp()) {
        if (pd == pdesc) {
            if (!pt1)
                sPrptyList = pd->next_prp();
            else
                pt1->set_next_prp(pd->next_prp());
            return (pdesc);
        }
    }
    return (0);
}


// Remove all properties that match value.
//
void
CDs::prptyRemove(int value)
{
    CDs *sd = owner();
    if (sd) {
        sd->prptyRemove(value);
        return;
    }

    CDp *pp = 0, *pn;
    for (CDp *p = sPrptyList; p; p = pn) {
        pn = p->next_prp();
        if (p->value() == value) {
            if (!pp)
                sPrptyList = pn;
            else
                pp->set_next_prp(pn);
            delete p;
            continue;
        }
        pp = p;
    }
}


// Remove all mention of odesc in the cell's properties, and in
// the bound labels (make them unbound) unless the label has the
// CDdeleted flag set.
//
void
CDs::prptyPurge(CDo *odesc)
{
    if (!odesc)
        return;

    CDs *sd = owner();
    if (sd) {
        sd->prptyPurge(odesc);
        return;
    }

    for (CDp *pdesc = sPrptyList; pdesc; pdesc = pdesc->next_prp())
        pdesc->purge(odesc);
    for (CDp *pd = odesc->prpty_list(); pd; pd = pd->next_prp()) {
        CDla *la = pd->bound();
        if (la && !(la->state() == CDDeleted)) {
            CDp_lref *prf = (CDp_lref*)la->prpty(P_LABRF);
            if (prf) {
                la->prptyUnlink(prf);
                delete prf;
            }
        }
    }
}


void
CDs::prptyFreeList()
{
    CDs *sd = owner();
    if (sd) {
        sd->prptyFreeList();
        return;
    }

    for (CDp *pd = sPrptyList; pd; pd = sPrptyList) {
        sPrptyList = pd->next_prp();
        delete pd;
    }
}


// Look through the cell reference points.  If any correspond to an
// element that was just moved, transform the reference.  True is
// returned if a change was made.
//
bool
CDs::prptyTransformRefs(const cTfmStack *tstk, CDo *odesc)
{
    if (!odesc)
        return (false);

    // 'this' can be symbolic.
    BBox tBB = odesc->oBB();
    bool changed = false;
    for (CDp *pdesc = prptyList(); pdesc; pdesc = pdesc->next_prp()) {
        int x, y, x1, y1, x2, y2;
        if (pdesc->value() == P_NODE) {
            // connection terminal
            CDp_snode *pn = (CDp_snode*)pdesc;
            if (isSymbolic()) {
                for (unsigned int ix = 0; ; ix++) {
                    if (!pn->get_pos(ix, &x, &y))
                        break;
                    if (tBB.intersect(x, y, true) && is_active(odesc, x, y)) {
                        // If the terminal is connected to odesc and not
                        // connected to something else, move it.
                        //
                        tstk->TPoint(&x, &y);
                        pn->set_pos(ix, x, y);
                        changed = true;
                    }
                }
            }
            else {
                pn->get_schem_pos(&x, &y);
                if (tBB.intersect(x, y, true) && is_active(odesc, x, y)) {
                    // If the terminal is connected to odesc and not
                    // connected to something else, move it.
                    //
                    tstk->TPoint(&x, &y);
                    pn->set_schem_pos(x, y);
                    changed = true;
                }
            }
        }
        else if (pdesc->value() == P_MUT) {
            // Get the transformed lower left corner.
            if (odesc->type() != CDINSTANCE)
                continue;
            CDp_mut *pm = (CDp_mut*)pdesc;
            x = pm->pos1_x();
            y = pm->pos1_y();
            x1 = pm->pos2_x();
            y1 = pm->pos2_y();
            if (x == tBB.left && y == tBB.bottom) {
                x2 = tBB.right;
                y2 = tBB.top;
                tstk->TPoint(&x, &y);
                tstk->TPoint(&x2, &y2);
                if (x > x2) x = x2;
                if (y > y2) y = y2;
            }
            else if (x1 == tBB.left && y1 == tBB.bottom) {
                x2 = tBB.right;
                y2 = tBB.top;
                tstk->TPoint(&x1, &y1);
                tstk->TPoint(&x2, &y2);
                if (x1 > x2) x1 = x2;
                if (y1 > y2) y1 = y2;
            }
            else
                break;
            pm->set_pos1_x(x);
            pm->set_pos1_y(y);
            pm->set_pos2_x(x1);
            pm->set_pos2_y(y1);
        }
    }
    return (changed);
}


// Similar to prptyInstLink below, but for wire node properties.
//
void
CDs::prptyWireLink(const cTfmStack *tstk, CDw *onew, CDw *old,
    CDmcType MoveOrCopy)
{
    CDs *sd = owner();
    if (sd) {
        sd->prptyWireLink(tstk, onew, old, MoveOrCopy);
        return;
    }

    if (old) {
        for (CDp *pd = old->prpty_list(); pd; pd = pd->next_prp()) {
            if (pd->bound())
                prptyLabelLink(tstk, onew, pd, MoveOrCopy);
        }
    }
}


// Set up the label linkage and mutual inductor pointers for the
// new instance.  Called when an instance is being moved or copied.
// The transform for the move/copy must be active.  Args old and new
// are calls to library device.  It is assumed that only one device
// of a mutual inductor pair is being copied, otherwise call
// CDLinkNewMutualPair().
//
void
CDs::prptyInstLink(const cTfmStack *tstk, CDc *onew, CDc *old,
    CDmcType MoveOrCopy)
{
    CDs *sd = owner();
    if (sd) {
        sd->prptyInstLink(tstk, onew, old, MoveOrCopy);
        return;
    }

    int mutref = false;
    if (old) {
        for (CDp *pd = old->prpty_list(); pd; pd = pd->next_prp()) {
            if (pd->bound())
                prptyLabelLink(tstk, onew, pd, MoveOrCopy);
            else if (pd->value() == P_MUTLRF)
                mutref = true;
        }
    }
    if (mutref) {
        // This is one of a mutual inductor pair
        if (MoveOrCopy == CDcopy) {
            // Remove the new MUTLRF properties, since we know that
            // the other inductor was not copied.
            //
            if (onew) {
                CDp *pn;
                for (CDp *pd = onew->prpty_list(); pd; pd = pn) {
                    pn = pd->next_prp();
                    if (pd->value() == P_MUTLRF) {
                        onew->prptyUnlink(pd);
                        delete pd;
                    }
                }
            }
            return;
        }
        CDp_nmut *pm = (CDp_nmut*)prpty(P_NEWMUT);
        for ( ; pm; pm = pm->next()) {
            // fix the appropriate pointer
            pm->updat_ref(old, onew);
            if (pm->l1_dev() == onew || pm->l2_dev() == onew) {
                // Now deal with the mutual inductor label
                CDla *olabel = pm->bound();
                if (!olabel)  // old label or 0
                    continue;
                Label label(olabel->la_label());
                pm->bind(0);
                if (!pm->l1_dev() || !pm->l2_dev())
                    continue;
                label.x = (pm->l1_dev()->oBB().left +
                    pm->l1_dev()->oBB().right + pm->l2_dev()->oBB().left +
                    pm->l2_dev()->oBB().right)/4;
                label.x -= label.width/2;
                label.y = (pm->l1_dev()->oBB().bottom +
                    pm->l1_dev()->oBB().top + pm->l2_dev()->oBB().bottom +
                    pm->l2_dev()->oBB().top)/4;
                label.y -= label.height/2;
                CDla *nlabel = newLabel(olabel, &label, olabel->ldesc(),
                    olabel->prpty_list(), true);
                if (!nlabel)
                    return;
                pm->bind(nlabel);
                nlabel->link(this, 0, 0);
            }
        }
    }
}


// This sets the internal pointers used with mutual inductors in
// electrical mode.
//
void
CDs::prptyInstPatch(CDc *cdesc)
{
    if (!cdesc || !cdesc->prpty(P_MUTLRF))
        return;

    CDs *sd = owner();
    if (sd) {
        sd->prptyInstPatch(cdesc);
        return;
    }

    CDp_name *pn = (CDp_name*)cdesc->prpty(P_NAME);
    if (!pn || !pn->name_string())
        return;

    int num = pn->number();
    const char *pname = pn->name_string()->string();

    // This is one of the 'new' mutual inductors.  Establish the
    // pointer.  If the pointer is already set, there is an error,
    // so remove the mutlrf property.
    //
    CDp_nmut *pm = (CDp_nmut*)prpty(P_NEWMUT);
    while (pm) {
        CDp *pd;
        if (pm->l1_index() == num && !strcmp(pm->l1_name(), pname)) {
            if (pm->l1_dev() && pm->l1_dev() != cdesc) {
                // Ack! remove one mutlef property
                pd = cdesc->prpty(P_MUTLRF);
                if (pd) {
                    cdesc->prptyUnlink(pd);
                    delete pd;
                }
                break;
            }
            else
                pm->set_l1_dev(cdesc);
        }
        else if (pm->l2_index() == num && !strcmp(pm->l2_name(), pname)) {
            if (pm->l2_dev() && pm->l2_dev() != cdesc) {
                // Ack! remove one mutlef property
                pd = cdesc->prpty(P_MUTLRF);
                if (pd) {
                    cdesc->prptyUnlink(pd);
                    delete pd;
                }
                break;
            }
            else
                pm->set_l2_dev(cdesc);
        }
        pm = pm->next();
    }
}


// Look through the properties and delete any label references found. 
// Also delete the mutual inductor references if the device was a
// mutual inductor.  This is called when deleting the odesc, applies
// presently to instances and wires.
//
void
CDs::prptyUnref(CDo *odesc)
{
    if (!odesc)
        return;

    CDs *sd = owner();
    if (sd) {
        sd->prptyUnref(odesc);
        return;
    }

    CDp *pd, *pn;
    bool mutref = false;
    for (pd = odesc->prpty_list(); pd; pd = pd->next_prp()) {
        CDla *olabel = pd->bound();
        CD()->ifRecordObjectChange(this, olabel, 0);
        if (pd->value() == P_MUTLRF)
            mutref = true;
    }
    if (mutref && odesc->type() == CDINSTANCE) {
        // It was a mutual inductor.  Delete the mutual inductors
        // containing this device.
        //
        for (pd = sPrptyList; pd; pd = pn) {
            pn = pd->next_prp();
            if (pd->value() == P_NEWMUT) {
                CDp_nmut *pm = (CDp_nmut*)pd;
                CDc *other = 0;
                if (pm->match((CDc*)odesc, &other)) {
                    // Found the reference, other points to the
                    // other inductor.
                    //
                    CDla *olabel = pm->bound();
                    CD()->ifRecordObjectChange(this, olabel, 0);
                    prptyUnlink(pd);
                    delete pd;
                    if (other) {
                        // delete one MUTLRF property
                        pd = other->prpty(P_MUTLRF);
                        CD()->ifRecordPrptyChange(this, other, pd, 0);
                    }
                }
            }
        }
    }
}


// Set up the label linkage pointer for the given property.  Delete the
// old label, and create new one.
// CDo *onew; (new instance or wire)
// CDp *oldp; (property from old instance or wire)
//
void
CDs::prptyLabelLink(const cTfmStack *tstk, CDo *onew, CDp *oldp,
    CDmcType MoveOrCopy)
{
    if (!onew || !oldp)
        return;

    CDs *sd = owner();
    if (sd) {
        sd->prptyLabelLink(tstk, onew, oldp, MoveOrCopy);
        return;
    }

    CDp *newp = onew->prpty(oldp->value());
    if (!newp)
        return;
    CDla *olabel = oldp->bound();
    if (!olabel)
        return;

    Label label(olabel->la_label());

    // The default is to translate the label according to the current
    // move, however the callback may change this.
    tstk->TPoint(&label.x, &label.y);

    if (!CD()->ifUpdatePrptyLabel(oldp->value(), onew, &label)) {
        // Update the label xform.
        cTfmStack stk;
        stk.TSetTransformFromXform(label.xform, 0, 0);
        GEO()->applyCurTransform(&stk, 0, 0, 0, 0);
        CDtf tf;
        stk.TCurrent(&tf);
        label.xform &= ~TXTF_XF;
        label.xform |= (tf.get_xform() & TXTF_XF);
        stk.TPop();
    }

    CDla *nlabel = newLabel(MoveOrCopy == CDcopy ? 0 : olabel, &label,
        olabel->ldesc(), olabel->prpty_list(), true);
    if (!nlabel)
        return;
    newp->bind(nlabel);
    nlabel->link(this, onew, newp);

    if (onew->type() == CDINSTANCE) {
        if (newp->value() == P_NAME && MoveOrCopy == CDcopy)
            // revert copy to default name
            CD()->ifUpdateNameLabel((CDc*)onew, (CDp_name*)newp);
    }
}


// If the label has a LABRF property, hunt for the corresponding
// instance (it should already be in the master list), mutual inductor
// or text block reference and establish the linking pointers.
// in electrical mode only, as the cell is being parsed.
//
// It is critical that instances be in memory before this function is
// called.  The CD package always writes instances before geometry, so
// this will always be true when reading files generated by CD.
//
void
CDs::prptyLabelPatch(CDla *odesc)
{
    if (!odesc)
        return;

    CDs *sd = owner();
    if (sd) {
        sd->prptyLabelPatch(odesc);
        return;
    }

    CDp_lref *pl = (CDp_lref*)odesc->prpty(P_LABRF);
    if (!pl)
        return;

    if (!pl->name()) {
        // Wire node label.
        CDw *wd = (CDw*)pl->find_my_object(this, odesc);
        if (!wd) {
            // Didn't find wire or property, remove labref.
            odesc->prptyRemove(P_LABRF);
        }
        return;
    }

    unsigned int num = pl->number();
    int prp_num = pl->propnum();

    if (prp_num == P_NEWMUT) {
        // mutual inductor reference
        CDp_nmut *pm = (CDp_nmut*)prpty(P_NEWMUT);
        while (pm && pm->index() != (int)num)
            pm = pm->next();
        if (pm) {
            pm->bind(odesc);
            odesc->link(this, 0, 0);
            return;
        }
        // Didn't find associated property, remove labref.
        odesc->prptyRemove(P_LABRF);
        return;
    }

    CDc *cdesc = (CDc*)pl->find_my_object(this, odesc);
    if (!cdesc) {
        // Didn't work out, sorry.
        odesc->prptyRemove(P_LABRF);
    }
}


// Update the property reference pointers of a new label which is to
// replace an old label.  Args new and old are labels.  If new is 0,
// old is being deleted.
//
bool
CDs::prptyLabelUpdate(CDla *onew, CDla *old)
{
    if (!old)
        return (false);

    CDs *sd = owner();
    if (sd)
        return (sd->prptyLabelUpdate(onew, old));

    CDp_lref *prf = (CDp_lref*)old->prpty(P_LABRF);
    if (prf) {
        CDp *pdesc = prf->propref();
        CDo *odesc = prf->devref();
        if (pdesc && odesc) {
            // reference from a library cell
            CDp *op = pdesc;
            pdesc = pdesc->dup();
            if (pdesc) {
                pdesc->bind(onew);
                CD()->ifRecordPrptyChange(this, odesc, op, pdesc);
                if (onew)
                    onew->link(this, odesc, pdesc);
            }
        }
        else if (odesc && odesc->type() == CDWIRE) {

            // Depending on the label, this will create either a node
            // or bnode property.
            //
            if (!((CDw*)odesc)->set_node_label(this, onew)) {
                Errs()->add_error(
                    "prptyLabelUpdate: failed to add new label.");
                return (false);
            }
            unsetConnected();
        }
        else if (odesc == (CDo*)this) {
            // reference from the symbol properties (newmut)
            if (prf->propnum() != P_NEWMUT)
                // shouldn't happen
                return (true);
            CDp_nmut *pm = (CDp_nmut*)prpty(P_NEWMUT);
            while (pm && pm->bound() != old)
                pm = pm->next();
            if (pm)
                pm->bind(onew);
            if (onew)
                onew->link(this, 0, 0);
        }
    }
    return (true);
}


// Add a mutual inductance coupling odesc1 and odesc2.  Arg val is the
// coupling coefficient, name is an overriding name if given.
//
bool
CDs::prptyMutualAdd(CDc *odesc1, CDc *odesc2, const char *val,
    const char *mname)
{
    if (!odesc1 || !odesc2)
        return (false);

    CDs *sd = owner();
    if (sd)
        return (sd->prptyMutualAdd(odesc1, odesc2, val, mname));

    CDp_name *pn1 = (CDp_name*)odesc1->prpty(P_NAME);
    CDp_name *pn2 = (CDp_name*)odesc2->prpty(P_NAME);
    if (!pn1 || !pn1->name_string() || !pn2 || !pn2->name_string()) {
        Errs()->add_error(
            "prptyMutualAdd: (internal error) unnmed inductor.");
        return (false);
    }
    CDp_nmut *pm = new CDp_nmut(0,
        lstring::copy(pn1->name_string()->string()), pn1->number(),
        lstring::copy(pn2->name_string()->string()), pn2->number(),
        lstring::copy(val), (mname && *mname) ? lstring::copy(mname) : 0);
    pm->set_l1_dev(odesc1);
    pm->set_l2_dev(odesc2);
    pm->set_next_prp(sPrptyList);
    sPrptyList = pm;
    if (!CD()->ifUpdateMutLabel(this, pm)) {
        Errs()->add_error("prptyMutualAdd: setMutLabel failed.");
        return (false);
    }
    odesc1->prptyAdd(P_MUTLRF, "mutual", Electrical);
    odesc2->prptyAdd(P_MUTLRF, "mutual", Electrical);
    return (true);
}


// Set up the label linkage and mutual inductor pointers for the
// mutual inductor pair currently being copied.  The copy transform
// must be active.  The args are calls, known to be a new style mutual
// inductor pair.  The pmo is the source mutual inductor property, not
// necessarily from the current cell.  This function is called for
// copy, not move.
//
void
CDs::prptyMutualLink(const cTfmStack *tstk, CDc *new1, CDc *new2,
    CDp_nmut *pmo)
{
    if (!pmo)
        return;

    CDs *sd = owner();
    if (sd) {
        sd->prptyMutualLink(tstk, new1, new2, pmo);
        return;
    }

    CDc *old1 = pmo->l1_dev();
    CDc *old2 = pmo->l2_dev();
    if (!old1 || !old2)
        return;
    if (!new1 || !new2)
        return;
    CDp *pd;
    for (pd = old1->prpty_list(); pd; pd = pd->next_prp()) {
        if (pd->bound())
            prptyLabelLink(tstk, new1, pd, CDcopy);
    }
    for (pd = old2->prpty_list(); pd; pd = pd->next_prp()) {
        if (pd->bound())
            prptyLabelLink(tstk, new2, pd, CDcopy);
    }

    CDp_nmut *pmn = (CDp_nmut*)prptyAddCopy(pmo);
    if (pmn) {
        pmn->set_l1_dev(new1);
        pmn->set_l2_dev(new2);
        pmn->set_index(0);
        // Use default name for copy.
        pmn->set_assigned_name(0);

        // Now deal with the new mutual inductor label
        CDla *olabel = pmo->bound();
        if (olabel) {
            Label label(olabel->la_label());
            bool copied;
            label.label = pmn->label_text(&copied);
            tstk->TPoint(&label.x, &label.y);
            CDla *nlabel = newLabel(0, &label, olabel->ldesc(),
                olabel->prpty_list(), true);
            if (copied)
                hyList::destroy(label.label);
            if (nlabel) {
                pmn->bind(nlabel);
                nlabel->link(this, 0, 0);
            }
        }
    }
}


// Once the instances have been updated with CDc::updateDeviceName(), this
// function updates the properties associated with mutual inductors.
//
void
CDs::prptyMutualUpdate()
{
    CDs *sd = owner();
    if (sd) {
        sd->prptyMutualUpdate();
        return;
    }

    int count = 1;
    CDp_nmut *pm = (CDp_nmut*)prpty(P_NEWMUT);
    for ( ; pm; pm = pm->next()) {
        CDc *odesc1, *odesc2;
        if (!pm->get_descs(&odesc1, &odesc2))
            // bad link, dead mut reference
            continue;
        CDp_name *pn = (CDp_name*)odesc1->prpty(P_NAME);
        if (pn)
            pm->set_l1_index(pn->number());
        pn = (CDp_name*)odesc2->prpty(P_NAME);
        if (pn)
            pm->set_l2_index(pn->number());
        CDla *olabel = pm->bound();  // the label
        if (olabel) {
            CDp_lref *pl = (CDp_lref*)olabel->prpty(P_LABRF);
            if (pl)
                pl->set_number(count);
        }
        pm->set_index(count);
        CD()->ifUpdateMutLabel(this, pm);
        count++;
    }
}


// Return true and the inductor odescs if the mutual inductor property
// is valid.
//
bool
CDs::prptyMutualFind(CDp *pdesc, CDc **od1, CDc **od2)
{
    if (!pdesc)
        return (false);

    CDs *sd = owner();
    if (sd)
        return (sd->prptyMutualFind(pdesc, od1, od2));

    if (pdesc->value() == P_NEWMUT) {
        CDp_nmut *pmn = (CDp_nmut*)pdesc;
        return (pmn->get_descs(od1, od2));
    }
    else if (pdesc->value() == P_MUT) {
        CDp_mut *pmo = (CDp_mut*)pdesc;
        // Old style property, search the "ind" instances for a match
        // with the LL corner of the bounding box.
        //
        CDm_gen mgen(this, GEN_MASTERS);
        CDm *mdesc;
        for (mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            const char *cn = mdesc->cellname()->string();
            if ((cn[0] == 'i' || cn[0] == 'I') &&
                    (cn[1] == 'n' || cn[1] == 'N') &&
                    (cn[2] == 'd' || cn[2] == 'D') && !cn[3])
                break;
        }
        if (!mdesc)
            return (false);
        int x1, y1, x2, y2;
        pmo->get_coords(&x1, &y1, &x2, &y2);
        CDc *odesc1 = 0;
        CDc *odesc2 = 0;
        CDc_gen cgen(mdesc);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
            if (c->oBB().left == x1 && c->oBB().bottom == y1)
                odesc1 = c;
            else if (c->oBB().left == x2 && c->oBB().bottom == y2)
                odesc2 = c;
            if (odesc1 && odesc2) {
                if (od1)
                    *od1 = odesc1;
                if (od2)
                    *od2 = odesc2;
                return (true);
            }
        }
    }
    return (false);
}


//
// Private functions
//

// Recursive core of fixBBs.  Check and set the bounding boxes of all
// cells in the hierarchy.  This has the side effect of
// checking/setting the cell pointers in the master list.
//
bool
CDs::fix_bb(ptrtab_t *tab, int depth)
{
    if (depth >= CDMAXCALLDEPTH) {
        Errs()->add_error(
            "fixBBs: hierarchy is too deep, probably recursive");
        return (false);
    }

    CDm_gen mgen(this, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            // "can't happen"
            continue;

        if (!tab->find(msdesc)) {
            if (!msdesc->fix_bb(tab, depth+1))
                return (false);
            tab->add(msdesc);
        }
    }

    db_insert_deferred_instances();
    checkInstances();

    setBBvalid(false);
    if (!computeBB())
        return (false);

    // The reflect function sets the BB and calls the db_reinsert
    // function for all instances of this.  If CD()->SetDeferInst was
    // active during the read, instances will be inserted here.  If
    // CD()->SetDeferInst is active now, instances will be added in
    // "deferred" mode, requiring a call to
    // db_insert_deferred_instances to finalize insertion.

    CD()->SetDeferInst(true);
    reflect();
    CD()->SetDeferInst(false);

    setBBsubng(false);
    return (true);
}


void
CDs::reflect_bad_BB()
{
    setBBsubng(true);
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        CDs *sd = m->parent();
        if (sd && !sd->isBBsubng())
            sd->reflect_bad_BB();
    }
}


void
CDs::reflect_bad_extract()
{
    setExtracted(false);
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        CDs *sd = m->parent();
        if (sd && sd->isExtracted())
            sd->reflect_bad_extract();
    }
}


void
CDs::reflect_bad_assoc()
{
    setAssociated(false);
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        CDs *sd = m->parent();
        if (sd && sd->isAssociated())
            sd->reflect_bad_assoc();
    }
}


void
CDs::reflect_bad_gplane()
{
    setGPinv(false);
    CDm_gen gen(this, GEN_MASTER_REFS);
    for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
        CDs *sd = m->parent();
        if (sd && sd->isGPinv())
            sd->reflect_bad_gplane();
    }
}


// Return true if x, y correspond to a connection point in odesc,
// and no other object.
//
bool
CDs::is_active(const CDo *odesc, int x, int y) const
{
    if (!odesc)
        return (false);

    bool active = false;
    if (odesc->type() == CDWIRE) {
        const Point *pts = ((CDw*)odesc)->points();
        int num = ((CDw*)odesc)->numpts();
        for (int i = 0; i < num; i++) {
            if (pts[i].x == x && pts[i].y == y) {
                active = true;
                break;
            }
        }
    }
    else if (odesc->type() == CDINSTANCE) {
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        for (; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int px, py;
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (px == x && py == y) {
                    active = true;
                    break;
                }
            }
            if (active)
                break;
        }
    }
    if (!active)
        return (false);

    BBox AOI;
    AOI.left = AOI.right = x;
    AOI.bottom = AOI.top = y;

    // First check the devices.
    CDg gdesc;
    gdesc.init_gen(this, CellLayer(), &AOI);
    CDo *pointer;
    while ((pointer = gdesc.next()) != 0) {
        if (!pointer->is_normal() || pointer == odesc)
            continue;
        CDp *pdesc = pointer->prpty_list();
        for (; pdesc; pdesc = pdesc->next_prp()) {
            if (pdesc->value() != P_NODE)
                continue;
            CDp_cnode *pn = (CDp_cnode*)pdesc;
            for (unsigned int ix = 0; ; ix++) {
                int px, py;
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (px == x && py == y)
                    return (false);
            }
        }
    }

    // Now check active wires.
    CDsLgen gen(this);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(this, ld, &AOI);
        while ((pointer = gdesc.next()) != 0) {
            if (pointer->type() != CDWIRE)
                continue;
            if (!pointer->is_normal() || pointer == odesc)
                continue;
            const Point *pts = ((CDw*)pointer)->points();
            int num = ((CDw*)pointer)->numpts();
            for (int i = 0; i < num; i++) {
                if (pts[i].x == x && pts[i].y == y)
                    return (false);
            }
        }
    }
    return (true);
}

