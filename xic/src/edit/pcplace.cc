
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
#include "edit.h"
#include "pcell.h"
#include "pcell_params.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "oa_if.h"
#include "errorlog.h"
#include "select.h"
#include "undolist.h"


//
// PCell instance placement and reparameterization.
//

// Pop up the parameter list for the passed instance.  If pstr is not
// null, it will return the new parameter string on success.
//
bool
cEdit::reparameterize(CDc *cd, char **pstr)
{
    if (pstr)
        *pstr = 0;
    if (!cd)
        return (false);
    CDp *pp = cd->prpty(XICP_PC_PARAMS);
    if (!pp)
        return (false);
    CDp *pn = cd->prpty(XICP_PC);
    if (!pn)
        return (false);

    char *dbname = PCellDesc::canon(pn->string());
    PCellParam *pm;
    if (PC()->getDefaultParams(dbname, &pm))
        pm->update(pp->string());
    else {
        Log()->ErrorLogV(mh::PCells,
            "Can't find default parameters for %s.", dbname);
        delete [] dbname;
        return (false);
    }
    delete [] dbname;

    bool ret = PopUpPCellParams(0, MODE_ON, pm, pn->string(), pcpEdit);
    if (ret && pstr)
        *pstr = pm->string(true);
    PCellParam::destroy(pm);
    return (ret);
}


// Exported.
//
bool
cEdit::openPlacement(const CDcbin *cbin, const char *dbname)
{
    return (startPlacement(cbin, dbname, pcpOpen));
}


// Begin placement or open of pcell sub-masters. 
//
// Presently, cbin contains the entry for native pcells, dbname is for
// OA.
//
// A false return should be considered as an error.  Native pcells
// should be verified with isSuperMaster before calling this.
//
bool
cEdit::startPlacement(const CDcbin *cbin, const char *dbname, pcpMode pmode)
{
    // Used for all PCells.

    CDs *sd = cbin ? cbin->celldesc(DSP()->CurMode()) : 0;
    if (sd) {
        if (!PC()->isSuperMaster(sd)) {
            Errs()->add_error("Cell %s is not a super-master.",
                Tstring(sd->cellname()));
            return (false);
        }

        // Xic native PCell.
        stopPlacement();
        ed_pcsuper = sd;
        char *dbn = PCellDesc::mk_native_dbname(
            Tstring(ed_pcsuper->cellname()));

        if (!PC()->getDefaultParams(dbn, &ed_pcparams))
            return (false);
        ed_pcparams->setup(PC()->curPCinstParams(dbn));
        if (pmode != pcpNone)
            PopUpPCellParams(0, MODE_ON, ed_pcparams, dbn, pmode);
        delete [] dbn;
    }
    else if (dbname) {
        stopPlacement();
        // The pcell is from OA.
        ed_pcsuper = 0;
        if (!PC()->getDefaultParams(dbname, &ed_pcparams))
            return (false);
        ed_pcparams->setup(PC()->curPCinstParams(dbname));
        if (pmode != pcpNone)
            PopUpPCellParams(0, MODE_ON, ed_pcparams, dbname, pmode);
    }
    else
        return (false);
    return (true);
}


// Handler for the "Reset" button in the parameter entry panel, this
// reverts to the default parameter set.
//
bool
cEdit::resetPlacement(const char *dbname)
{
    // Used for all PCells.

    if (ed_pcsuper) {
        // Xic native PCell.
        PCellParam *pdef;
        if (!PC()->getDefaultParams(dbname, &pdef))
            return (false);
        PCellParam *px = ed_pcparams;
        ed_pcparams = pdef;
        PopUpPCellParams(0, MODE_UPD, ed_pcparams, 0, pcpNone);
        char *dbn = PCellDesc::mk_native_dbname(
            Tstring(ed_pcsuper->cellname()));
        PC()->setPCinstParams(dbn, ed_pcparams, false);
        delete [] dbn;
        PCellParam::destroy(px);
    }
    else if (dbname) {
        // The pcell is from OA.
        PCellDesc *pd = PC()->findSuperMaster(dbname);
        if (pd && pd->defaultParams()) {
            PCellParam *px = ed_pcparams;
            ed_pcparams = PCellParam::dup(pd->defaultParams());
            PopUpPCellParams(0, MODE_UPD, ed_pcparams, 0, pcpNone);
            PC()->setPCinstParams(dbname, ed_pcparams, false);
            PCellParam::destroy(px);
        }
    }
    return (true);
}


// End instance placement, pop down the parameter setting panel.
//
void
cEdit::stopPlacement()
{
    // Used for all PCells.

    PopUpPCellParams(0, MODE_OFF, 0, 0, pcpNone);
    ed_pcsuper = 0;
    PCellParam::destroy(ed_pcparams);
    ed_pcparams = 0;
}


// This is called just before cell placement, or opening if openmode
// is true.  If we are placing a PCell, rebuild the sub-master, as the
// parameters may have changed.
//
bool
cEdit::resolvePCell(CDcbin *cbin, const char *dbname, bool openmode)
{
    // Used for all PCells.

    // If no parameters, assume that we are not placing a pcell, and
    // just return happy.
    if (!ed_pcparams)
        return (true);

    if (ed_pcsuper) {
        // Placing Xic native PCell.
        if (!cbin) {
            Errs()->add_error(
                "resolvePCell: internal error, null argument.");
            return (false);
        }
        if (ed_pcsuper->isElectrical()) {
            // can't happen
            Errs()->add_error(
                "resolvePCell: super-master %s is not physical.",
                Tstring(ed_pcsuper->cellname()));
            return (false);
        }

        char *dbn = PCellDesc::mk_native_dbname(
            Tstring(ed_pcsuper->cellname()));
        PC()->setPCinstParams(dbn, ed_pcparams, false);

        CDs *sdsub;
        char *prms = ed_pcparams->string(true);
        OItype oiret = PC()->openSubMaster(ed_pcsuper, prms, &sdsub);
        if (oiret != OIok) {
            Errs()->add_error(
                "resolvePCell: failed to generate sub-master of "
                "%s.\nParams: %s", Tstring(ed_pcsuper->cellname()),
                prms ? prms : "null");
            delete [] prms;
            return (false);
        }
        delete [] prms;
        cbin->setPhys(sdsub);
        return (true);
    }

    if (!dbname) {
        Errs()->add_error("resolvePCell: null database name.");
        return (false);
    }

    // This is called for all cells, so ignore cells not in our
    // pcell table.
    if (!PC()->findSuperMaster(dbname))
        return (true);

    // OA imported PCell.
    char *libname, *cellname, *viewname;
    if (!PCellDesc::split_dbname(dbname, &libname, &cellname, &viewname)) {
        Errs()->add_error("resolvePCell: database name not canonical.");
        return (false);
    }
    PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

    char *subm_name;
    if (!OAif()->load_cell(libname, cellname, viewname, CDMAXCALLDEPTH,
            openmode, &ed_pcparams, &subm_name)) {
        Errs()->add_error("resolvePCell: OA cell load failed.");
        return (false);
    }
    if (cbin) {
        if (OIfailed(CD()->OpenExisting(subm_name, cbin))) {
            delete [] subm_name;
            Errs()->add_error("resolvePCell: failed to open cell.");
            return (false);
        }
    }
    PC()->setPCinstParams(dbname, ed_pcparams, false);
    delete [] subm_name;
    return (true);
}


// Apply a new set of parameters to sdesc, which is a sub-master of a
// PCell.  The sub-master is cleared and rebuilt with the new params. 
// All instances of this cell will be updated.
//
bool
cEdit::reparamSubMaster(CDs *sdesc, const char *inprms)
{
    // Used for all PCells.

    if (!sdesc)
        return (false);
    if (!sdesc->isPCellSubMaster()) {
        Errs()->add_error(
        "reparamSubMaster: attempt to apply parameters to non-sub-master.");
        return (false);
    }
    CDp *pn = sdesc->prpty(XICP_PC);
    if (!pn) {
        Errs()->add_error("reparamSubMaster: missing pc_name property.");
        return (false);
    }
    CDp *pp = sdesc->prpty(XICP_PC_PARAMS);
    if (!pp) {
        Errs()->add_error("reparamSubMaster: missing pc_params property.");
        return (false);
    }

    // We can determine from the name whether the super-master in from
    // OA or not.
    char *libname, *cellname, *viewname;
    if (!PCellDesc::split_dbname(pn->string(), &libname, &cellname,
            &viewname)) {
        // Must be Xic native.

        char *dbname = PCellDesc::mk_native_dbname(
            lstring::strip_path(pn->string()));
        if (!PCellDesc::split_dbname(dbname, &libname, &cellname, &viewname)) {
            // can't happen
            delete [] dbname;
            return (false);
        }
        PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

        PCellParam *prms;
        if (!PC()->getParams(dbname, inprms, &prms)) {
            delete [] dbname;
            return (false);
        }

        PCellParam *oldprms;
        if (!PC()->getParams(dbname, pp->string(), &oldprms)) {
            delete [] dbname;
            PCellParam::destroy(prms);
            return (false);
        }

        if (*oldprms == *prms) {
            // No change, we're done.
            delete [] dbname;
            PCellParam::destroy(prms);
            PCellParam::destroy(oldprms);
            return (true);
        }

        PCellDesc *pd = PC()->findSuperMaster(dbname);
        if (!pd) {
            Errs()->add_error(
                "reparamSubMaster: super-master not in table.");
            delete [] dbname;
            PCellParam::destroy(prms);
            PCellParam::destroy(oldprms);
            return (false);
        }
        PCellItem *piold = pd->findItem(oldprms);
        if (!piold) {
            Errs()->add_error(
                "reparamSubMaster: sub-master not in item list.");
            delete [] dbname;
            PCellParam::destroy(prms);
            PCellParam::destroy(oldprms);
            return (false);
        }
        PCellItem *pinew = pd->findItem(prms);
        char *oldcellname = 0;
        if (pinew) {
            // We already have a sub-master for the new parameter set. 
            // The existing sub-master will become a duplicate not in
            // the table, as we will remove the entry for the old
            // parameter set.

            pd->removeItem(piold);

            // Temporarily set the cellname, we will revert this.
            oldcellname = lstring::copy(pinew->cellname());
            pinew->setCellname(Tstring(sdesc->cellname()));
        }
        else {
            // Change the existing parameter set associated with this
            // item.

            piold->setParams(prms);
        }

        // Open super-master.
        CDs *sdsup;
        OItype oiret = PC()->openMaster(pn->string(), sdesc->displayMode(),
            &sdsup);
        if (oiret != OIok) {
            Errs()->add_error("reparamSubMaster: failed to open master %s.",
                pn->string());
            delete [] dbname;
            PCellParam::destroy(prms);
            PCellParam::destroy(oldprms);
            if (pinew) {
                pinew->setCellname(oldcellname);
                delete [] oldcellname;
            }
            return (false);
        }
        if (!PC()->isSuperMaster(sdsup)) {
            Errs()->add_error(
                "reparamSubMaster: cell %s is not a valid super-master.",
                pn->string());
            delete [] dbname;
            PCellParam::destroy(prms);
            PCellParam::destroy(oldprms);
            if (pinew) {
                pinew->setCellname(oldcellname);
                delete [] oldcellname;
            }
            return (false);
        }

        char *prmstr = prms->string(true);
        char *nmstr = lstring::copy(pn->string());

        ulPCstate pcstate;
        EditIf()->ulPCreparamSet(&pcstate);
        sdesc->prptyRemove(XICP_PC_PARAMS);
        sdesc->prptyRemove(XICP_PC);
        CDp *ptmp = sdesc->prptyList();
        sdesc->setPrptyList(0);
        sdesc->clear(true);
        sdesc->setPrptyList(ptmp);

        sdsup->cloneCell(sdesc);
        sdesc->prptyRemove(XICP_PC_PARAMS);
        sdesc->prptyAdd(XICP_PC_PARAMS, prmstr);
        delete [] prmstr;

        bool ret = PC()->evalScript(sdesc, nmstr);
        delete [] nmstr;
        delete [] dbname;
        PCellParam::destroy(prms);
        PCellParam::destroy(oldprms);
        EditIf()->ulPCreparamReset(&pcstate);

        if (pinew) {
            pinew->setCellname(oldcellname);
            delete [] oldcellname;
        }

        if (!ret) {
            Errs()->add_error("reparamSubMaster: script evaluation failed.");
            return (false);
        }
    }
    else {
        // Super-master is in OA.

        PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

        PCellParam *prms;
        if (!PC()->getParams(pn->string(), inprms, &prms))
            return (false);

        PCellParam *oldprms;
        if (!PC()->getParams(pn->string(), pp->string(), &oldprms)) {
            PCellParam::destroy(prms);
            return (false);
        }

        if (*oldprms == *prms) {
            // No change, we're done.
            PCellParam::destroy(prms);
            PCellParam::destroy(oldprms);
            return (true);
        }

        PCellDesc *pd = PC()->findSuperMaster(pn->string());
        if (!pd) {
            Errs()->add_error(
                "reparamSubMaster: super-master not in table.");
            PCellParam::destroy(prms);
            PCellParam::destroy(oldprms);
            return (false);
        }
        PCellItem *piold = pd->findItem(oldprms);
        if (!piold) {
            Errs()->add_error(
                "reparamSubMaster: sub-master not in item list.");
            PCellParam::destroy(prms);
            PCellParam::destroy(oldprms);
            return (false);
        }
        PCellItem *pinew = pd->findItem(prms);
        char *oldcellname = 0;
        if (pinew) {
            // We already have a sub-master for the new parameter set. 
            // The existing sub-master will become a duplicate not in
            // the table, as we will remove the entry for the old
            // parameter set.

            pd->removeItem(piold);

            // Temporarily set the cellname, we will revert this.
            oldcellname = lstring::copy(pinew->cellname());
            pinew->setCellname(Tstring(sdesc->cellname()));
        }
        else {
            // Change the existing parameter set associated with this
            // item.

            piold->setParams(prms);
        }

        // Clear the cell, but keep any properties that are not PCell
        // properties (pcell properties will be put back by OA).

        sdesc->prptyRemove(XICP_PC_PARAMS);
        sdesc->prptyRemove(XICP_PC);
        CDp *ptmp = sdesc->prptyList();
        sdesc->setPrptyList(0);
        sdesc->clear(true);
        sdesc->setPrptyList(ptmp);

        // Now call back to OA to fill in the cell.

        bool ret = OAif()->load_cell(libname, cellname, viewname,
            CDMAXCALLDEPTH, false, &prms, 0);

        // Put back the previous instance name if we changed it.
        // Future instances with this parameter set will use the
        // previous name.

        if (pinew) {
            pinew->setCellname(oldcellname);
            delete [] oldcellname;
        }

        PCellParam::destroy(prms);
        PCellParam::destroy(oldprms);
        if (!ret) {
            Errs()->add_error("reparamSubMaster: OA cell load failed.");
            return (false);
        }
    }

    // We need to go through all of the instances and update the
    // PC_PARAMS property.  Some subtlety here - this will be called
    // on undo, so we don't use the undo list here.

    pp = sdesc->prpty(XICP_PC_PARAMS);
    if (pp) {
        CDm_gen gen(sdesc, GEN_MASTER_REFS);
        for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
            CDc_gen cgen(m);
            for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
                CDp *pd = cdesc->prpty(XICP_PC_PARAMS);
                if (pd)
                    pd->set_string(pp->string());
            }
        }
    }

    if (sdesc == CurCell()) {
        EditIf()->unregisterGrips(0);
        EditIf()->registerGrips(0);
    }
    return (true);
}


// Handle a new XICP_PC_PARAMS property applied to an instance of a
// pcell sub-master.
//
bool
cEdit::reparamInstance(CDs *sdesc, CDc *cdesc, const CDp *newp, CDc **pnew)
{
    // Used for all PCells.
    if (pnew)
        *pnew = 0;

    if (!newp || newp->value() != XICP_PC_PARAMS) {
        Errs()->add_error("reparamInstance: null or incorrect property type.");
        return (false);
    }
    if (cdesc->is_copy()) {
        Errs()->add_error(
            "reparamInstance: can't apply pc_params to an instance copy.");
        return (false);
    }
    CDs *msdesc = cdesc->masterCell();
    if (!msdesc) {
        Errs()->add_error(
            "reparamInstance: Internal error, instance with no master!");
        return (false);
    }
    if (!msdesc->isPCellSubMaster()) {
        Errs()->add_error(
            "reparamInstance: attempt to apply params to non-pcell instance.");
        return (false);
    }
    CDp *pn = cdesc->prpty(XICP_PC);
    if (!pn)
        pn = msdesc->prpty(XICP_PC);
    if (!pn) {
        Errs()->add_error("reparamInstance: can't determine pcell name!");
        return (false);
    }

    bool was_selected = (cdesc->state() == CDSelected);
    CDs *sdsub = 0;

    // We can determine from the name whether the super-master in from
    // OA or not.
    char *libname, *cellname, *viewname;
    if (!PCellDesc::split_dbname(pn->string(), &libname, &cellname,
            &viewname)) {
        // Must be Xic native.

        char *dbname = PCellDesc::mk_native_dbname(
            lstring::strip_path(pn->string()));
        if (!PCellDesc::split_dbname(dbname, &libname, &cellname, &viewname)) {
            // can't happen
            delete [] dbname;
            return (false);
        }
        PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

        CDp *p_prms = cdesc->prpty(XICP_PC_PARAMS);
        if (!p_prms)
            p_prms = msdesc->prpty(XICP_PC_PARAMS);
        if (!p_prms) {
            Errs()->add_error(
                "reparamInstance: missing pc_params property.");
            return (false);
        }

        PCellParam *prms;
        if (!PC()->getDefaultParams(dbname, &prms)) {
            delete [] dbname;
            return (false);
        }
        prms->update(p_prms->string());
        prms->update(newp->string());

        delete [] dbname;
        char *prmstr = prms->string(true);
        PCellParam::destroy(prms);
#ifdef PC_DEBUG
        printf("reparamInstance: prmstr = %s\n", prmstr);
#endif

        CDs *sdsup;
        OItype oiret = PC()->openMaster(pn->string(), sdesc->displayMode(),
            &sdsup);
        if (oiret != OIok) {
            Errs()->add_error(
                "reparamInstance: failed to open super-master %s.",
                pn->string());
            delete [] prmstr;
            return (false);
        }
        if (!PC()->isSuperMaster(sdsup)) {
            Errs()->add_error("reparamInstance: invalid super-master %s.",
                pn->string());
            delete [] prmstr;
            return (false);
        }

        oiret = PC()->openSubMaster(sdsup, prmstr, &sdsub);
        if (oiret != OIok) {
            Errs()->add_error(
                "reparamInstance: failed to generate sub-master of "
                "%s.\nParams: %s",
                Tstring(sdsup->cellname()), prmstr ? prmstr : "null");
            delete [] prmstr;
            return (false);
        }
        delete [] prmstr;
#ifdef PC_DEBUG
        CDp *px = sdsub->prpty(XICP_PC_PARAMS);
        printf("reparamInstance:  sdsub = %s\n", px ? px->string() : "null");
#endif
    }
    else {
        // Get a sub-master from OA.

        PCellDesc::LCVcleanup lcv(libname, cellname, viewname);

        CDp *p_prms = cdesc->prpty(XICP_PC_PARAMS);
        if (!p_prms)
            p_prms = msdesc->prpty(XICP_PC_PARAMS);
        if (!p_prms) {
            Errs()->add_error("reparamInstance: missing pc_params property.");
            return (false);
        }

        PCellParam *prms;
        if (!PC()->getDefaultParams(pn->string(), &prms))
            return (false);
        prms->update(p_prms->string());
        prms->update(newp->string());

        // Now call back to OA to fill in the cell.

        char *subm_name;
        if (!OAif()->load_cell(libname, cellname, viewname, CDMAXCALLDEPTH,
                false, &prms, &subm_name)) {
            Errs()->add_error("reparamInstance: OA cell load failed.");
            return (false);
        }

        CDcbin cbin;
        if (OIfailed(CD()->OpenExisting(subm_name, &cbin))) {
            delete [] subm_name;
            Errs()->add_error("reparamInstance: failed to open cell.");
            return (false);
        }
        delete [] subm_name;
        PCellParam::destroy(prms);

        sdsub = cbin.celldesc(sdesc->displayMode());
    }

    CallDesc calldesc(sdsub->cellname(), sdsub);

    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(cdesc);
    CDtx tx;
    stk.TCurrent(&tx);
    stk.TPop();
    CDap ap(cdesc);
    CDc *newc;
    if (OIfailed(sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone, &newc))) {
        Errs()->add_error("reparamInstance: makeCall failed.");
        return (false);
    }
    for (CDp *pd = cdesc->prpty_list(); pd; pd = pd->next_prp()) {
        if (pd->value() == XICP_PC || pd->value() == XICP_PC_PARAMS)
            continue;
        newc->prptyAddCopy(pd);
    }
#ifdef PC_DEBUG
    CDp *px = newc->prpty(XICP_PC_PARAMS);
    printf("reparamInstance:  newcd = %s\n", px ? px->string() : "null");
#endif

    CD()->ifRecordObjectChange(sdesc, cdesc, newc);
    if (was_selected)
        Selections.insertObject(sdesc, newc);
    if (pnew)
        *pnew = newc;
    return (true);
}


// Replace cdesc with a new instantiation, using the prm_value for the
// parameter whose name is passed.
//
bool
cEdit::resetInstance(CDc *cdesc, const char *prm_name, double prm_value)
{
    if (cdesc) {
        CDp *p_prms = cdesc->prpty(XICP_PC_PARAMS);
        if (!p_prms) {
            Errs()->add_error("Instance has no parameters property.");
            return (false);
        }

        PCellParam *prms;
        if (!PCellParam::parseParams(p_prms->string(), &prms))
            return (false);

        if (!prms->setValue(prm_name, prm_value)) {
            PCellParam::destroy(prms);
            return (false);
        }

        char *nstr = prms->string(true);
        CDp *nprp = new CDp(nstr, XICP_PC_PARAMS);
        delete [] nstr;
        PCellParam::destroy(prms);

        Ulist()->ListCheck("PRMCHG", CurCell(), true);
        Ulist()->RecordPrptyChange(CurCell(), cdesc, p_prms, nprp);
        Ulist()->CommitChanges(true);
    }
    else {
        CDp *p_prms = CurCell()->prpty(XICP_PC_PARAMS);
        if (!p_prms) {
            Errs()->add_error("Current cell has no parameters property.");
            return (false);
        }

        PCellParam *prms;
        if (!PCellParam::parseParams(p_prms->string(), &prms))
            return (false);

        if (!prms->setValue(prm_name, prm_value)) {
            PCellParam::destroy(prms);
            return (false);
        }

        char *nstr = prms->string(true);
        CDp *nprp = new CDp(nstr, XICP_PC_PARAMS);
        delete [] nstr;
        PCellParam::destroy(prms);

        Ulist()->ListCheck("PRMCHG", CurCell(), false);
        Ulist()->RecordPrptyChange(CurCell(), 0, p_prms, nprp);
        Ulist()->CommitChanges();
    }
    return (true);
}

