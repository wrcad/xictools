
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

#include "cd.h"
#include "cd_types.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "fio.h"
#include "fio_library.h"
#include <unistd.h>


// Generator - it is ok to call on a recursive hierarchy.
//
CDgenHierDn_cbin::CDgenHierDn_cbin(const CDcbin *cbin)
{
    if (!cbin) {
        dp = -1;
        tab = 0;
        return;
    }
    sdescs[0] = cbin->phys();
    if (!sdescs[0])
        sdescs[0] = cbin->elec();
    if (!sdescs[0]) {
        dp = -1;
        tab = 0;
        return;
    }
    generators[0].init(sdescs[0]->masters(), false);
    dp = 0;
    tab = new ptrtab_t;
    tab->add(cbin);
}


bool
CDgenHierDn_cbin::next(CDcbin *cbin, bool *err)
{
    if (err)
        *err = false;
    for (;;) {
        if (dp < 0)
            return (0);
        CDm *m = generators[dp].next();
        if (!m) {
            if (sdescs[dp]->isElectrical() ||
                    !CDcdb()->findCell(sdescs[dp]->cellname(), Electrical)) {
                dp--;
                CDcdb()->findSymbol(sdescs[dp+1]->cellname(), cbin);
                return (true);
            }
            sdescs[dp] = CDcdb()->findCell(sdescs[dp]->cellname(), Electrical);
            generators[dp].init(sdescs[dp]->masters(), false);
            continue;
        }
        CDs *msdesc = m->celldesc();
        if (!msdesc)
            continue;
        if (!m->hasInstances())
            continue;
        // The cellnames in the masters and cells are kept in the string
        // table, so each name has a unique address.  This is used to
        // mark visited in the tab.
        if (tab->find(msdesc->cellname()))
            continue;
        dp++;
        if (dp == CDMAXCALLDEPTH) {
            if (err)
                *err = true;
            return (0);
        }
        tab->add(msdesc->cellname());
        sdescs[dp] = msdesc;
        generators[dp].init(msdesc->masters(), false);
    }
}
// End of CDgenHierDn_cbin functions


// Constructor.  If a cell desc is given, the CDcbin will be
// initialized with the argument and its opposite-mode counterpart, if
// it exists in memory.
//
CDcbin::CDcbin(CDs *sd)
{
    reset();
    if (sd) {
        if (sd->isElectrical()) {
            cbElecDesc = sd;
            cbPhysDesc = CDcdb()->findCell(sd->cellname(), Physical);
        }
        else {
            cbPhysDesc = sd;
            cbElecDesc = CDcdb()->findCell(sd->cellname(), Electrical);
        }
    }
}


// Constructor.  The cell pointers are set from the cell tables, as
// keyed by sname.
//
CDcbin::CDcbin(CDcellName sname)
{
    reset();
    if (sname) {
        cbPhysDesc = CDcdb()->findCell(sname, Physical);
        cbElecDesc = CDcdb()->findCell(sname, Electrical);
    }
}


void
CDcbin::setPhys(CDs *sd)
{
    if (sd && sd->isElectrical()) {
        CD()->DbgError("non-phys", "CDcbin::setElec");
        return;
    }
    cbPhysDesc = sd;
}


void
CDcbin::setElec(CDs *sd)
{
    if (sd) {
        if (!sd->isElectrical()) {
            CD()->DbgError("non-elec", "CDcbin::setElec");
            return;
        }
        if (sd->isSymbolic()) {
            sd = sd->owner();
            CD()->DbgError("symbolic", "CDcbin::setElec");
        }
    }
    cbElecDesc = sd;
}


// Return the cell name.  All cell names are derived from a string
// table, thus 1) a cell never has a null name, and 2) value comparison
// can be used to establish name equivalence.
//
// In particular, if cellname() returns null, we know that both cell
// pointers are null.
//
CDcellName
CDcbin::cellname() const
{
    if (cbPhysDesc)
        return (cbPhysDesc->cellname());
    if (cbElecDesc)
        return (cbElecDesc->cellname());
    return (0);
}


FileType
CDcbin::fileType() const
{
    if (cbPhysDesc) {
        FileType ft = cbPhysDesc->fileType();
        if (ft != Fnone)
            return (ft);
    }
    if (cbElecDesc)
        return (cbElecDesc->fileType());
    return (Fnone);
}


void
CDcbin::setFileType(FileType ft)
{
    if (cbPhysDesc)
        cbPhysDesc->setFileType(ft);
    if (cbElecDesc)
        cbElecDesc->setFileType(ft);
}


const char *
CDcbin::fileName() const
{
    if (cbPhysDesc) {
        const char *fn = cbPhysDesc->fileName();
        if (fn)
            return (fn);
    }
    if (cbElecDesc)
        return (cbElecDesc->fileName());
    return (0);
}


void
CDcbin::setFileName(const char *fn)
{
    if (cbPhysDesc)
        cbPhysDesc->setFileName(fn);
    if (cbElecDesc)
        cbElecDesc->setFileName(fn);
}


// Update all of the filename pointers in the hierarchy, skipping
// library cells.
//
void
CDcbin::updateFileName(const char *fname)
{
    CDarchiveName fn = CD()->ArchiveTableAdd(fname);
    CDgenHierDn_cbin gen(this);
    CDcbin cbin;
    while (gen.next(&cbin, 0)) {
        if (cbin.isLibrary())
            continue;
        if (cbin.phys())
            cbin.phys()->setFileName(fn);
        if (cbin.elec())
            cbin.elec()->setFileName(fn);
    }
}


bool
CDcbin::isUnread() const
{
    return ((!cbPhysDesc || cbPhysDesc->isUnread()) &&
        (!cbElecDesc || cbElecDesc->isUnread()));
}


bool
CDcbin::isEmpty() const
{
    return ((!cbPhysDesc || cbPhysDesc->isEmpty()) &&
        (!cbElecDesc || cbElecDesc->isEmpty()));
}


bool
CDcbin::isSubcell() const
{
    return ((cbPhysDesc && cbPhysDesc->isSubcell()) ||
        (cbElecDesc && cbElecDesc->isSubcell()));
}


bool
CDcbin::isModified() const
{
    return ((cbPhysDesc && cbPhysDesc->countModified()) ||
        (cbElecDesc && cbElecDesc->countModified()));
}


bool
CDcbin::isCompressed() const
{
    return ((cbPhysDesc && cbPhysDesc->isCompressed()) ||
        (cbElecDesc && cbElecDesc->isCompressed()));
}


bool
CDcbin::isSaventv() const
{
    return ((cbPhysDesc && cbPhysDesc->isSaventv()) ||
        (cbElecDesc && cbElecDesc->isSaventv()));
}


bool
CDcbin::isAltered() const
{
    return ((cbPhysDesc && cbPhysDesc->isAltered()) ||
        (cbElecDesc && cbElecDesc->isAltered()));
}


bool
CDcbin::isDevice() const
{
    return ((cbPhysDesc && cbPhysDesc->isDevice()) ||
        (cbElecDesc && cbElecDesc->isDevice()));
}


bool
CDcbin::isLibrary() const
{
    return ((cbPhysDesc && cbPhysDesc->isLibrary()) ||
        (cbElecDesc && cbElecDesc->isLibrary()));
}


bool
CDcbin::isImmutable() const
{
    return ((cbPhysDesc && cbPhysDesc->isImmutable()) ||
        (cbElecDesc && cbElecDesc->isImmutable()));
}


bool
CDcbin::isOpaque() const
{
    return ((cbPhysDesc && cbPhysDesc->isOpaque()) ||
        (cbElecDesc && cbElecDesc->isOpaque()));
}


bool
CDcbin::isConnector() const
{
    return ((cbPhysDesc && cbPhysDesc->isConnector()) ||
        (cbElecDesc && cbElecDesc->isConnector()));
}


bool
CDcbin::isUflag(int i) const
{
    return ((cbPhysDesc && cbPhysDesc->isUflag(i)) ||
        (cbElecDesc && cbElecDesc->isUflag(i)));
}


bool
CDcbin::isArchiveTopLevel() const
{
    return ((cbPhysDesc && cbPhysDesc->isArchiveTopLevel()) ||
        (cbElecDesc && cbElecDesc->isArchiveTopLevel()));
}


void
CDcbin::setCompressed(bool b)
{
    if (cbPhysDesc)
        cbPhysDesc->setCompressed(b);
    if (cbElecDesc)
        cbElecDesc->setCompressed(b);
}


void
CDcbin::setSaventv(bool b)
{
    if (cbPhysDesc)
        cbPhysDesc->setSaventv(b);
    if (cbElecDesc)
        cbElecDesc->setSaventv(b);
}


void
CDcbin::setAltered(bool b)
{
    if (cbPhysDesc)
        cbPhysDesc->setAltered(b);
    if (cbElecDesc)
        cbElecDesc->setAltered(b);
}


void
CDcbin::setDevice(bool b)
{
    // Electrical only.
    if (cbElecDesc)
        cbElecDesc->setDevice(b);
}


void
CDcbin::setLibrary(bool b)
{
    if (cbPhysDesc)
        cbPhysDesc->setLibrary(b);
    if (cbElecDesc)
        cbElecDesc->setLibrary(b);
}


void
CDcbin::setImmutable(bool b)
{
    if (cbPhysDesc)
        cbPhysDesc->setImmutable(b);
    if (cbElecDesc)
        cbElecDesc->setImmutable(b);
}


void
CDcbin::setOpaque(bool b)
{
    if (cbPhysDesc)
        cbPhysDesc->setOpaque(b);
    if (cbElecDesc)
        cbElecDesc->setOpaque(b);
}


void
CDcbin::setConnector(bool b)
{
    if (cbPhysDesc)
        cbPhysDesc->setConnector(b);
    if (cbElecDesc)
        cbElecDesc->setConnector(b);
}


void
CDcbin::setUflag(int i, bool b)
{
    if (cbPhysDesc)
        cbPhysDesc->setUflag(i, b);
    if (cbElecDesc)
        cbElecDesc->setUflag(i, b);
}


cGroupDesc *
CDcbin::groups()
{
    if (cbPhysDesc)
        return (cbPhysDesc->groups());
    return (0);
}


cNodeMap *
CDcbin::nodes()
{
    if (cbElecDesc)
        return (cbElecDesc->nodes());
    return (0);
}


// Check/set the bounding boxes of the physical/electrical cells used
// in the hierarchy for either mode.  This will also check/set master
// desc cell pointers and a few other things.
//
// This is not the same as calling CDs::fixBBs on the physical and
// electrical parts, which will miss cells that don't appear under the
// root cell.
//
bool
CDcbin::fixBBs()
{
    bool ret = true;
    ptrtab_t *tp = new ptrtab_t;
    ptrtab_t *te = new ptrtab_t;
    CDgenHierDn_cbin gen(this);
    CDcbin cbin;
    bool err;
    while ((gen.next(&cbin, &err)) != 0) {
        if (cbin.phys()) {
            if (!cbin.phys()->fixBBs(tp)) {
                ret = false;
                break;
            }
        }
        if (cbin.elec()) {
            if (!cbin.elec()->fixBBs(te)) {
                ret = false;
                break;
            }
        }
    }
    delete tp;
    delete te;
    if (err)
        ret = false;
    return (ret);
}


namespace {
    // Give each of the masters a new name, and set the modified state of
    // the parents.  The name should be a stable copy!
    //
    void
    rename_master_refs(CDs *sdesc, CDcellName name)
    {
        CDm_gen gen(sdesc, GEN_MASTER_REFS);
        for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
            CDs *parent = m->parent();
            if (!parent) {
                m->setCellname(name);
                continue;
            }
            m->unlink();
            m->setCellname(name);
            parent->linkMaster(m);
            parent->incModified();
        }
    }
}


// Change the name of the cell(s) in the database, if possible.
// Return true on success, false otherwise.
//
bool
CDcbin::rename(const char *newname)
{
    if (!newname) {
        Errs()->add_error("rename: null cellname given");
        return (false);
    }

    // Strip leading and trailing white space.  Allow embedded white
    // space for now.
    char *nn = lstring::copy(newname);
    char *e = nn + strlen(nn) - 1;
    while (e >= nn && isspace(*e))
        *e-- = 0;
    GCarray<char*> gc_nn(nn);
    newname = nn;
    while (isspace(*newname))
        newname++;
    if (!*newname) {
        Errs()->add_error("rename: empty cellname given");
        return (false);
    }

    if (CDcdb()->findSymbol(CD()->CellNameTableFind(newname))) {
        Errs()->add_error("rename: name %s is in use", newname);
        return (false);
    }
    // Check for clash with unopened lib cell
    if (FIO()->LookupLibCell(0, newname, LIBdevice, 0)) {
        Errs()->add_error("rename: name %s is in use as device", newname);
        return (false);
    }

    // Can't change name if immutable.
    if (isImmutable()) {
        Errs()->add_error("rename: cell %s is immutable", Tstring(cellname()));
        return (false);
    }

    // Remove library association if any.
    setLibrary(false);

    CDcellName nname = CD()->CellNameTableAdd(newname);

    if (cbPhysDesc) {
        CDcdb()->unlinkCell(cbPhysDesc);
        cbPhysDesc->setName(nname);
        CDcdb()->linkCell(cbPhysDesc);
        rename_master_refs(cbPhysDesc, cbPhysDesc->cellname());
        cbPhysDesc->incModified();
    }
    if (cbElecDesc) {
        CDcdb()->unlinkCell(cbElecDesc);
        cbElecDesc->setName(nname);
        CDcdb()->linkCell(cbElecDesc);
        rename_master_refs(cbElecDesc, cbElecDesc->cellname());
        cbElecDesc->incModified();
    }

    setFileName(0);

    // Keep the same file type, except for OpenAccess.
    if (fileType() == Foa)
        setFileType(Fnone);

    return (true);
}


// Delete the cell(s), after deleting all instances, but only if all
// instances can be deleted.  Instances can't be deleted from
// immutable parents.
//
// True is returned if all instances and cells were deleted, false
// otherwise (this is not really an error).
//
bool
CDcbin::deleteCells()
{
    bool ret = true;
    if (cbPhysDesc) {
        bool keep = false;
        CDm_gen gen(cbPhysDesc, GEN_MASTER_REFS);
        for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
            if (!m->clear())
                keep = true;
        }
        if (!keep) {
            delete cbPhysDesc;
            cbPhysDesc = 0;
        }
        else
           ret = false;
    }
    if (cbElecDesc) {
        bool keep = false;
        CDm_gen gen(cbElecDesc, GEN_MASTER_REFS);
        for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
            if (!m->clear())
                keep = true;
        }
        if (!keep) {
            delete cbElecDesc;
            cbElecDesc = 0;
        }
        else
            ret = false;
    }
    return (ret);
}


// Return a list of the names of empty cells found under this (but not
// this itself).  The names retgurned are empty or absent in both modes.
//
stringlist *
CDcbin::listEmpties() const
{
    SymTab *empties = new SymTab(false, false);

    CDgenHierDn_cbin gen(this);
    CDcbin cbin;
    bool err;
    while (gen.next(&cbin, &err)) {
        if (cbin == *this)
            continue;
        if (cbin.isEmpty())
            empties->add(Tstring(cbin.cellname()), 0, false);
    }

    stringlist *sl = SymTab::names(empties);
    delete empties;
    return (sl);
}


// Return true if there is non-empty electrical data somewhere in the
// hierarchy.  This need not be rooted to the top cell, e.g.,
// consider a hierarchy that is entirely physical, except for one cell
// deep in the hierarchy which has electrical data.
//
bool
CDcbin::hasElectrical() const
{
    CDgenHierDn_cbin gen(this);
    CDcbin cbin;
    while (gen.next(&cbin, 0)) {
        CDs *sdesc = cbin.elec();
        if (sdesc && !sdesc->isEmpty())
            return (true);
    }
    return (false);
}


// Set all the Save Native flags in the hierarchy.
//
void
CDcbin::setSaveNative()
{
    CDgenHierDn_cbin gen(this);
    CDcbin cbin;
    while (gen.next(&cbin, 0)) {
        if (!cbin.isLibrary())
            cbin.setSaventv(true);
    }
}

