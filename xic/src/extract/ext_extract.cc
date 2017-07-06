
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
 $Id: ext_extract.cc,v 5.46 2016/06/27 15:58:16 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_grpgen.h"
#include "ext_nets.h"
#include "ext_errlog.h"
#include "ext_ufb.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "cd_propnum.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "tech_layer.h"
#include "events.h"
#include "layertab.h"
#include "pcell_params.h"
#include <algorithm>

#define TIME_DBG
#ifdef TIME_DBG
#include "timedbg.h"
#endif


//========================================================================
//
// Functions for device extraction and hierarchical connectivity
// establishment.
//
//========================================================================

//#define PERM_DEBUG


// Perform extraction for the current cell hierarchy.  This processes the
// entire hierarchy.
//
bool
cExt::extract(CDs *sdesc)
{
    if (!sdesc)
        return (true);
    if (sdesc->isElectrical())
        return (false);

    sDevDesc *d = EX()->deadDevices();
    EX()->setDeadDevices(0);
    while (d) {
        sDevDesc *dn = d->next();
        delete d;
        d = dn;
    }

    if (sdesc->isExtracted())
        return (true);

    XIrt ret = XIok;
    // Note: cellnames are all in name table.

    CDgenHierDn_s gen1(sdesc);
    bool err;
    CDs *sd;
    while ((sd = gen1.next(&err)) != 0)
        sd->setExtracted(false);
    if (err)
        return (false);

    // The entire hierarchy must be grouped.
    if (!group(sdesc, CDMAXCALLDEPTH))
        return (false);

    dspPkgIf()->SetWorking(true);
    if (EX()->isVerbosePromptline())
        PL()->PushPrompt("Extracting: ");
    else
        PL()->ShowPrompt("Extracting ...");
    ExtErrLog.start_logging(ExtLogExt, sdesc->cellname());

    cGroupDesc *gd = sdesc->groups();
    if (gd) {
        updateReferenceTable(0);
        ret = gd->setup_extract();
        if (ret == XIok) {
            EX()->updateReferenceTable(sdesc);
            if (EX()->isMergePhysContacts())
                gd->fix_connections();
        }
    }
#ifdef TIME_DBG
    Tdbg()->print_accum("extract_devs");
    Tdbg()->print_accum("connect_to_subs");
    Tdbg()->print_accum("connect_btwn_subs");
    Tdbg()->print_accum("other_stuff");
    Tdbg()->print_accum("direct_contact");
    Tdbg()->print_accum("layer_contact");
    Tdbg()->print_accum("via_contact");
    Tdbg()->print_accum("connect_total");
#endif

    ExtErrLog.add_log(ExtLogExt, "Extracting %s complete, %s.",
        sdesc->cellname()->string(),
        ret == XIok ? "no errors" : "error(s) encountered");
    ExtErrLog.end_logging();

    if (EX()->isVerbosePromptline())
        PL()->PopPrompt();
    if (ret == XIok) {
        if (EX()->isVerbosePromptline())
            PL()->ShowPromptV("Extraction complete in %s.",
                sdesc->cellname()->string());
    }
    else
        PL()->ShowPrompt("Extraction aborted.");
    dspPkgIf()->SetWorking(false);

    return (ret == XIok);
}


// Show the extraction-related highlighting for display update.
//
void
cExt::showExtract(WindowDesc *wdesc, bool d_or_e)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    cGroupDesc *gd = cursdp->groups();
    if (!gd || gd->isempty())
        return;
    if (ext_showing_groups)
        gd->show_groups(wdesc, d_or_e);
    if (ext_showing_devs)
        gd->show_devs(wdesc, d_or_e);
}


namespace {
    // Return true if the name matches a token in the flattenable list. 
    // These are cells that the user has specified as not independent for
    // extraction, but rather should be considered as part of the
    // containing cell.  These are usually low-level cells containing all
    // or part of a device.
    //
    // The flatten list contains space-separated tokens of the form
    // [/]name[/], where
    //   name[/]    name is a matched as a prefix
    //   /name      name is matched as a suffix
    //   /name/     name is matched literally
    //
    bool flt_match(const char *name, const char *flist)
    {
        const char *s = flist;
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            char *ntok = tok;
            int mode = 0;
            if (*ntok == '/') {
                mode += 2;
                ntok++;
            }
            char *e = ntok + strlen(ntok) - 1;
            if (*e == '/') {
                mode++;
                *e = 0;
            }
            if (mode <= 1) {
                if (lstring::prefix(ntok, name)) {
                    delete [] tok;
                    return (true);
                }
            }
            else if (mode == 2) {
                if (lstring::suffix(ntok, name)) {
                    delete [] tok;
                    return (true);
                }
            }
            else {
                if (lstring::eq(ntok, name)) {
                    delete [] tok;
                    return (true);
                }
            }
        }
        return (false);
    }
}


// Return true if the instance should be flattened.  This applies to
// both electrical and physical instances.  If an electrical instance
// is flattened, structure in the master will be promoted to the
// current schematic before LVS comparison.
//
// The parent is the (effective) container cell of the instance.  This
// is passed because the cdesc->parent() may be incorrect if cdesc is
// a copy.
//
bool
cExt::shouldFlatten(const CDc *cdesc, CDs *parent)
{
    if (!cdesc || !parent)
        return (false);
    if (parent->isElectrical())
        return (sElecNetList::should_flatten(cdesc, parent));

    CDs *mstr = cdesc->masterCell();
    if (!mstr)
        return (false);

    // Note the logic for master and instance flatten properties. 
    // It the master has the property, it works as "not-flatten"
    // for the instance.
    //
    bool fm = mstr->prpty(XICP_EXT_FLATTEN);
    bool fi = cdesc->prpty(XICP_EXT_FLATTEN);
    if ((fm && !fi) || (!fm && fi))
        return (true);
    if (fm || fi)
        return (false);

    if (mstr->groups() && mstr->groups()->test_nets_only())
        return (true);


    if (cdesc->in_db()) {
        // For an ordinary instance, if the corresponding electrical
        // parent does not contain a matching instance, we assume that
        // the instance should be flattened.
        //
        CDcbin pcbin(parent);
        if (pcbin.elec() && !pcbin.elec()->isEmpty()) {
            bool found = false;
            CDm_gen mgen(pcbin.elec(), GEN_MASTERS);
            for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
                if (md->cellname() == mstr->cellname()) {
                    if (md->hasInstances())
                        found = true;
                    break;
                }
            }
            if (!found)
                return (true);
        }
    }
    else {
        // If cdesc is a copy, meaning it has been added from
        // flattening another subcell, flatten if there are no
        // unpaired instances of this type.

// If the instance has been promoted due to flattening, flatten this
// one too.  this will flatten all subcircuits under the
// initially-flattened instance, which was the original flattening
// algorithm.  May want to make this an option.  It should work, but
// can be horribly inefficient.
//
#ifdef FLATTEN_ALL
        return (true);
#endif

        int ninst = 0;
        CDcbin pcbin(parent);
        if (pcbin.elec() && !pcbin.elec()->isEmpty()) {
            CDm_gen mgen(pcbin.elec(), GEN_MASTERS);
            for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
                if (md->cellname() == mstr->cellname()) {
                    CDc_gen cgen(md);
                    for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                        CDp_range *pr = (CDp_range*)c->prpty(P_RANGE);
                        if (pr)
                            ninst += pr->width();
                        else
                            ninst++;
                    }
                    break;
                }
            }
            if (!ninst)
                return (true);
        }
        CDm_gen mgen(pcbin.phys(), GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
            if (md->cellname() == mstr->cellname()) {
                CDc_gen cgen(md);
                for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                    CDap ap(c);
                    ninst -= ap.nx*ap.ny;
                }
                break;
            }
        }
        if (ninst <= 0)
           return (true);
    }

    // If mstr is a pcell sub-master, find the electrical CDs whose
    // name will be provided in the property.  If this is a device,
    // then flatten.  This will be the case for all pcell devices.
    //
    CDp *pd = mstr->prpty(XICP_PC);
    if (pd) {
        char *cellname;
        if (PCellDesc::split_dbname(pd->string(), 0, &cellname, 0)) {
            CDs *esd = CDcdb()->findCell(cellname, Electrical);
            delete [] cellname;
            if (esd && esd->elecCellType() == CDelecDev)
                return (true);
        }
    }

    // Also flatten if the electrical counterpart is a device.
    CDs *esd = CDcdb()->findCell(mstr->cellname(), Electrical);
    if (esd && esd->elecCellType() == CDelecDev)
        return (true);

    // Check the "flatten prefix" list.
    return (flt_match(mstr->cellname()->string(), flattenPrefix()));
}


// Set up the "reference table".  The table is keyed by physical
// cells, and the payload is a list of group descs where the cell is
// used as a subcircuit.  The table understands the extraction
// flattening, unlike the MASTER_REFS list.
//
void
cExt::updateReferenceTable(const CDs *sd)
{
    if (ext_reference_tab) {
        SymTabGen gen(ext_reference_tab, true);
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            sGdList::destroy((sGdList*)ent->stData);
            delete ent;
        }
        delete ext_reference_tab;
        ext_reference_tab = 0;
    }

    if (!sd)
        return;
    cGroupDesc *gd = sd->groups();
    if (!gd)
        return;

    SymTab done_tab(false, false);
    ext_reference_tab = new SymTab(false, false);
    gd->list_callers(ext_reference_tab, &done_tab);
}


// Return a linked list of group descriptions that each call the
// passed cell as a subcell.
//
sGdList *
cExt::referenceList(const CDs *sdesc)
{
    if (!ext_reference_tab || !sdesc)
        return (0);
    sGdList *gdl = (sGdList*)ext_reference_tab->get((unsigned long)sdesc);
    if (gdl == (sGdList*)ST_NIL)
        return (0);
    return (gdl);
}


// If sd is physical, return true if extraction should be skipped.  If
// sd is electrical, return true if this cell should be omitted from
// netlist output.
//
bool
cExt::skipExtract(CDs *sd)
{
    if (sd->isElectrical()) {
        if (sd->isDevice() || sd->prpty(P_VIRTUAL))
            return (true);
        CDcbin cbin(sd);
        if (cbin.phys()) {
            if (cbin.phys()->isConnector())
                return (true);
            if (cbin.phys()->isOpaque() && !ext_extract_opaque)
                return (true);
        }
    }
    else {
        if (sd->isOpaque() && !ext_extract_opaque)
            return (true);
    }
    return (false);
}
// End of cExt functions.


// This is the main function for extracting devices and subcircuits, and
// adding them to the connectivity structures.
//
XIrt
cGroupDesc::setup_extract(int dcnt)
{
    if (dcnt >= CDMAXCALLDEPTH)
        return (XIbad);

    // In an opaque cell, skip the substructure, if any.  The groups
    // will contain the formal terminals and the implementations.
    // EX()->isExtractOpaque() overrides this.
    //
    if (!EX()->skipExtract(gd_celldesc)) {

        // Have to work from the bottom up.
        CDm_gen mgen(gd_celldesc, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
            CDs *msdesc = md->celldesc();
            if (!msdesc)
                continue;
            cGroupDesc *gd = msdesc->groups();
            if (gd) {
                XIrt ret = gd->setup_extract(dcnt + 1);
                if (ret != XIok)
                    return (ret);
            }
        }
    }
    if (gd_celldesc->isExtracted())
        return (XIok);

    if (ExtErrLog.log_extracting() && ExtErrLog.log_fp()) {
        FILE *fp = ExtErrLog.log_fp();
        fprintf(fp,
            "\n=======================================================\n");
        fprintf(fp, "Extracting cell %s\n",
            gd_celldesc->cellname()->string());
    }
    clear_extract();

    // Ensure that the cell's physical terminal database is
    // constructed and initialized.

    if (!gd_celldesc->checkPhysTerminals(true))
        ExtErrLog.add_err(Errs()->get_error());

    // Only the TE_FIXED terminals are considered here.  We attempt to
    // make the group identification.  The presence of these terminals
    // is important for series merging, or to ensure that subcircuits
    // have a connection and are therefor not ignored.  The rest of
    // the terminals are moved into place after association.

    CDpin *cterms = list_cell_terms(true);
    cterms = group_terms(cterms, true);
    // cterms contains ungrouped terms only

    // In an opaque cell, skip the substructure, if any.  The groups
    // will contain the formal terminals and the implementations.
    // EX()->isExtractOpaque() overrides this.
    //
    if (!EX()->skipExtract(gd_celldesc)) {

        // Add devices and subcircuits.
        EX()->activateGroundPlane(true);
        XIrt ret = add_subckts();
        if (ret != XIok) {
            clear_extract();
            EX()->activateGroundPlane(false);
            return (ret);
        }

        // Flatten flattenable subcircuits, i.e., promote their
        // structure into this cell.
        if (!flatten()) {
            clear_extract();
            EX()->activateGroundPlane(false);
            return (XIbad);
        }

        // Renumber subckts, since flattening affects numbering.
        sort_and_renumber_subs();
    }

    // The nets now have the objects for flattened subcells.  This
    // allows finding net name labels placed over flattened subcell
    // metal.
    //
    find_set_net_names();

    if (!EX()->isIgnoreNetLabels() && EX()->isMergeMatchingNamed()) {
        // Combine groups with the same name.
        int ndone = 0;
        SymTab tab(false, false);
        for (int i = 0; i < gd_asize; i++) {
            CDnetName nm = gd_groups[i].netname();
            if (!nm)
                continue;

            // If we find a named non-global wire-only net, don't
            // merge it.  To do so causes trouble in the gwl_drv_x64
            // extraction, not sure that I understand why.  We keep
            // the label, since at the end of association we may need
            // to place a terminal on it (found by name).
            //
            // There should be no problem merging global nets, and in
            // fact these may be needed for busk contact stamping.

            // The global flag in the group has not been officially
            // set yet.
            if (gd_groups[i].is_wire_only() &&
                    !SCD()->isGlobalNetName(nm->string()))
                continue;

            long onum = (long)tab.get((unsigned long)nm);
            if (onum >= 0) {
                ExtErrLog.add_log(ExtLogExt,
                    "Merging groups %d and %d named %s by labels.",
                    onum, i, gd_groups[onum].netname());
                reduce(onum, i);
                ndone++;
                i--;
                continue;
            }
            tab.add((unsigned long)nm, (void*)(long)i, false);
        }
        if (ndone)
            renumber_groups();
    }
    if (!EX()->isIgnoreNetLabels() && ExtErrLog.log_extracting()) {
        int i;
        for (i = 1; i < gd_asize; i++) {
            if (gd_groups[i].netname())
                break;
        }
        if (i < gd_asize) {
            ExtErrLog.add_log(ExtLogExt, "Group names from labels:");
            for ( ; i < gd_asize; i++) {
                CDnetName name = gd_groups[i].netname();
                if (!name)
                    continue;
                ExtErrLog.add_log(ExtLogExt, "  %3d  %s", i, name->string());
            }
        }
    }
    for (int i = 0; i < gd_asize; i++) {
        if (gd_groups[i].net())
            gd_groups[i].net()->set_length();
    }

    cterms = group_terms(cterms, false);
    CDpin::destroy(cterms);

    compute_cap();
    if (!EX()->skipExtract(gd_celldesc))
        EX()->activateGroundPlane(false);

    // Sort and renumber the devices.
    sort_and_renumber_devs();

    split_devices();
    combine_devices();

    // This creates subcircuit templates and consistently orders
    // instance connections.
    //
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next())
            s->update_template();
    }

    gd_celldesc->setExtracted(true);
    if (EX()->skipExtract(gd_celldesc))
        gd_celldesc->setAssociated(true);

    if (ExtErrLog.log_extracting() && ExtErrLog.log_fp())
        dump(ExtErrLog.log_fp());

    return (XIok);
}


// Destroy the extraction-related part of the group struct, but leave the
// groups intact.  Duality is gone.
//
void
cGroupDesc::clear_extract()
{
    {
        cGroupDesc *gdt = this;
        if (!gdt)
            return;
    }

    clear_duality();

    if (gd_celldesc == CurCell(Physical)) {
        if (EX()->isShowingDevs()) {
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0)
                show_devs(wdesc, ERASE);
        }
    }

    // Put all terminals back into an uninitialized state.
    CDcbin cbin(gd_celldesc);
    if (cbin.elec()) {
        CDp_snode *ps = (CDp_snode*)cbin.elec()->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            CDsterm *term = ps->cell_terminal();
            if (term)
                term->set_ref(0);
        }

        // Warning:
        // If we are destroying the cell hierarchy, beware that the
        // functions below call CDc::masterCell() which can regenerate
        // "missing" masters, which have already been freed.  There
        // are functions in CDcdb to prevent this.

        EX()->arrangeTerms(&cbin, false);
        EX()->arrangeInstLabels(&cbin);
    }

    for (int i = 0; i < gd_asize; i++) {
        sGroup &g = gd_groups[i];
        CDpin::destroy(g.termlist());
        g.set_termlist(0);
        sDevContactList::destroy(g.device_contacts());
        g.set_device_contacts(0);
        sSubcContactList::destroy(g.subc_contacts());
        g.set_subc_contacts(0);
        g.set_node(-1);
        g.set_netname(0, sGroup::NameFromLabel);
    }
    if (gd_e_phonycell) {
        if (gd_groups) {
            for (int i = 0; i < gd_asize; i++)
                gd_groups[i].purge_e_phonies();
        }
        delete gd_e_phonycell;
        gd_e_phonycell = 0;
    }
    sDevList::destroy(gd_devices);
    gd_devices = 0;
    sSubcList::destroy(gd_subckts);
    gd_subckts = 0;
    sVContact::destroy(gd_vcontacts);
    gd_vcontacts = 0;

    for (CDm *m = gd_master_list; m; m = gd_master_list) {
        gd_master_list = (CDm*)m->tab_next();
        m->setParent(0);
        CDc_gen cgen(m);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
            m->unlinkCdesc(c, false);
            c->setMaster(0);
            delete c;
        }
        delete m;
    }

    delete gd_flatten_tab;
    gd_flatten_tab = 0;
    stringlist::destroy(gd_lvs_msgs);
    gd_lvs_msgs = 0;

    // Clear the cached pin layer in the conductor layer descs.  The
    // pin purpose might have changed.
    CDextLgen gen(CDL_CONDUCTOR);
    CDl *ld;
    while ((ld = gen.next()) != 0)
        tech_prm(ld)->set_pin_layer(0);

    gd_celldesc->reflectBadExtract();
}


// Return the matching sSubcInst.
//
sSubcInst *
cGroupDesc::find_subc(CDc *c, int x, int y)
{
    cGroupDesc *gdt = this;
    if (gdt) {
        CDs *msdesc = c->masterCell(true);
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            if (sl->subs()->cdesc()->masterCell(true) != msdesc)
                continue;
            for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                if (c == s->cdesc() && x == s->ix() && y == s->iy())
                    return (s);
            }
            break;
        }
    }
    return (0);
}


// Compute the capacitance of each wire net, using the capacitance
// info from the layers.
//
void
cGroupDesc::compute_cap()
{
    int numl = CDldb()->layersUsed(Physical);
    Zlist **heads = new Zlist*[numl];
    memset(heads, 0, numl*sizeof(Zlist*));

    for (int i = 1; i < gd_asize; i++) {
        CDl *ldlast = 0;
        int ix = -1;
        bool hasnet = false;
        sGroupObjs *gobj = gd_groups[i].net();
        if (gobj) {
            for (CDol *ol = gobj->objlist(); ol; ol = ol->next) {
                CDl *ld = ol->odesc->ldesc();
                if (!ld->isConductor())
                    continue;
                if (tech_prm(ld)->cap_per_area() <= 0.0 &&
                        tech_prm(ld)->cap_per_perim() <= 0.0)
                    continue;
                if (ld != ldlast)
                    ix = ld->index(Physical);
                ldlast = ld;
                if (ix < 0)
                    continue;
                Zlist *z = ol->odesc->toZlist();
                if (z) {
                    Zlist *ze = z;
                    while (ze->next)
                        ze = ze->next;
                    ze->next = heads[ix];
                    heads[ix] = z;
                    hasnet = true;
                }
            }
        }
        double cap = 0.0;
        if (hasnet) {
            for (int itmp = 0; itmp < numl; itmp++) {
                if (heads[itmp]) {
                    CDl *ld = CDldb()->layer(itmp, Physical);
                    PolyList *p0 = Zlist::to_poly_list(heads[itmp]);
                    heads[itmp] = 0;
                    for (PolyList *p = p0; p; p = p->next) {
                        cap += p->po.area() * tech_prm(ld)->cap_per_area() +
                            MICRONS(p->po.perim()) *
                            tech_prm(ld)->cap_per_perim();
                    }
                    PolyList::destroy(p0);
                }
            }
        }
        gd_groups[i].set_capac(cap);
    }
    delete [] heads;
}


// Return true if the cell contains only wire, or subcells that
// contain only wire.  These will be flattened.
//
bool
cGroupDesc::test_nets_only()
{
    if (EX()->skipExtract(gd_celldesc))
        return (false);
    if (gd_celldesc->isConnector())
        return (true);
    if (gd_devices)
        return (false);
    if (gd_subckts) {
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            CDc *cdesc = sl->subs()->cdesc();
            CDs *sdesc = cdesc->masterCell(true);
            cGroupDesc *gd = sdesc->groups();
            if (gd) {
                if (!gd->test_nets_only())
                    return (false);
            }
        }
    }
    return (true);
}


// Recursive function to add cGroupDesc callers to the list_tab.  The
// done_tab should be provided by the caller.
//
void
cGroupDesc::list_callers(SymTab *list_tab, SymTab *done_tab)
{
    done_tab->add((unsigned long)gd_celldesc, 0, false);

    for (sSubcList *s = gd_subckts; s; s = s->next()) {
        CDs *sd = s->subs()->cdesc()->masterCell();
        if (!sd)
            continue;
        if (done_tab->get((unsigned long)sd) == ST_NIL) {
            cGroupDesc *gd = sd->groups();
            if (!gd) {
                done_tab->add((unsigned long)sd, 0, false);
                continue;
            }
            gd->list_callers(list_tab, done_tab);
        }
        if (list_tab) {
            SymTabEnt *ent = list_tab->get_ent((unsigned long)sd);
            if (!ent)
                list_tab->add((unsigned long)sd, new sGdList(this, 0), false);
            else
                ent->stData = new sGdList(this, (sGdList*)ent->stData);
        }
    }
}


// The terminal is assumed (but is not required to be) fixed and
// ungrouped.  Try to identify a group that can be bound to at the
// terminal's location, consistent with the layer hint if any.
//
// If a group can be inferred (i.e., the location is over
// layer-matched ROUTING metal that defines a group at the top
// level):
//
// 1. If the metal is at the top level:  set the terminal reference
//    object, and add the terminal to the group termlist.
//
// 2. If the metal is in a subcell, set the virtual group and the
//    layer (if it wasn't set) in the terminal, and add the terminal
//    to the group termlist.
//
// True is returned if a group was found.  If no group was found,
// the terminal is uninitialized, but retains any layer hint.
//
bool
cGroupDesc::bind_term_group_at_location(CDsterm *term)
{
    if (!term)
        return (false);

    // If the terminal is already bound to a group, unbind it.
    unbind_term(term);

    // Rid bogus hint layer.
    if (term->layer() && !term->layer()->isConductor())
        term->set_layer(0);

    // This is a little bit 'o magic that makes sure that any
    // connection within a subcell at the location has a path to the
    // top level, i.e., inter-hierarchy connections are added and new
    // groups created as necessary.
    //
    // Terminals over subcells that are virtual may not have a group
    // assigned at this point.  The group will be assigned here (if
    // possible) for fixed terminals.  Other such terminals will be
    // bound with a new group when placed, after association.
    //
    find_group_at_location(term->layer(), term->lx(), term->ly(), true);

    if (!term->layer() || term->layer()->isRouting()) {
        for (int i = 1; i < gd_asize; i++) {
            if (!gd_groups[i].net())
                continue;
            if (!gd_groups[i].net()->BB().intersect(term->lx(), term->ly(),
                    true))
                continue;
            for (CDol *ol = gd_groups[i].net()->objlist(); ol; ol = ol->next) {
                if (!ol->odesc->ldesc()->isRouting())
                    continue;
                if ((!term->layer() || term->layer() == ol->odesc->ldesc()) &&
                        ol->odesc->intersect(term->lx(), term->ly(), true)) {
                    term->set_ref(ol->odesc);
                    bind_term(term, term->group());
                    return (true);
                }
            }
        }
        if (gd_groups[0].net() &&
                gd_groups[0].net()->BB().intersect(term->lx(), term->ly(),
                true)) {
            // check ground group last
            for (CDol *ol = gd_groups[0].net()->objlist(); ol; ol = ol->next) {
                if (!ol->odesc->ldesc()->isRouting())
                    continue;
                if ((!term->layer() || term->layer() == ol->odesc->ldesc()) &&
                        ol->odesc->intersect(term->lx(), term->ly(), true)) {
                    term->set_ref(ol->odesc);
                    bind_term(term, term->group());
                    return (true);
                }
            }
        }
        for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
            for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                if (!s->cdesc()->oBB().intersect(term->lx(), term->ly(), true))
                    continue;

                cTfmStack stk;
                sSubcInstList *tsl = new sSubcInstList(s, 0);
                bool ret = bind_term_group_at_location_rc(tsl, term, stk);
                delete tsl;
                stk.TPop();
                if (ret)
                    return (true);
            }
        }
    }

    // Nothing found.  We will allow a CONDUCTOR at the top level,
    // with a warning.

    for (int i = 1; i < gd_asize; i++) {
        if (!gd_groups[i].net())
            continue;
        if (!gd_groups[i].net()->BB().intersect(term->lx(), term->ly(), true))
            continue;
        for (CDol *ol = gd_groups[i].net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isConductor())
                continue;
            if ((!term->layer() || term->layer() == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(term->lx(), term->ly(), true)) {
                term->set_ref(ol->odesc);
                warn_conductor(i, term->name()->string());
                bind_term(term, term->group());
                return (true);
            }
        }
    }
    if (gd_groups[0].net() &&
            gd_groups[0].net()->BB().intersect(term->lx(), term->ly(), true)) {
        // check ground group last
        for (CDol *ol = gd_groups[0].net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isConductor())
                continue;
            if ((!term->layer() || term->layer() == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(term->lx(), term->ly(), true)) {
                term->set_ref(ol->odesc);
                warn_conductor(0, term->name()->string());
                bind_term(term, term->group());
                return (true);
            }
        }
    }
    return (false);
}


// Call this from the top cell after extraction.  This will
// recursively examine subcell contact lists and combine contacts when
// possible.  This will account for split nets, where the net is
// actually completed through multiple contacts to the parent cell.
//
void
cGroupDesc::fix_connections()
{
    SymTab done_tab(false, false);
    for (sSubcList *s = gd_subckts; s; s = s->next()) {
        CDs *sd = s->subs()->cdesc()->masterCell();
        if (!sd)
            continue;
        if (done_tab.get((unsigned long)sd) != ST_NIL)
            continue;
        cGroupDesc *gd = sd->groups();
        if (!gd)
            continue;
        gd->fix_connections_rc(&done_tab);
    }
}


//
// The remaining cGroupDesc functions are private.
//

// Attempt to group the terminals in-place.  Successfully grouped
// terminals are removed from the list, which is returned.
//
CDpin *
cGroupDesc::group_terms(CDpin *pins, bool all)
{
    CDpin *pn, *pp = 0;
    for (CDpin *p = pins; p; p = pn) {
        pn = p->next();
        if (all)
            p->term()->set_ref(0);
        else {
            if (p->term()->group() >= 0) {
                // already grouped
                if (!pp)
                    pins = pn;
                else
                    pp->set_next(pn);
                delete p;
                continue;
            }
        }
        if (p->term()->is_fixed() && bind_term_group_at_location(p->term())) {
            if (!pp)
                pins = pn;
            else
                pp->set_next(pn);
            delete p;
            continue;
        }
        pp = p;
    }
    return (pins);
}


// Set the oGroup field of each cdesc to a number used for ordering
// in the cell-cell connectivity test.
//
void
cGroupDesc::set_subc_group(bool set)
{
    // Note that the numbering is in database order, for what its
    // worth.

    int count = 0;
    CDg gdesc;
    gdesc.init_gen(gd_celldesc, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        cdesc->set_group(count);
        if (set) {
            CDap ap(cdesc);
            count += ap.nx * ap.ny;
        }
    }
}


// The main function to process connectivity between subcircuits.
//
XIrt
cGroupDesc::add_subckts()
{
#ifdef TIME_DBG
    Tdbg()->start_timing("connect_total");
    Tdbg()->start_timing("extract_devs");
#endif
    // set the group numbers in subcells
    set_subc_group(true);

    // extract devices
    XIrt ret = add_devs();
    if (ret != XIok)
        return (ret);

    cTfmStack stk;
    sSubcLink *cl = build_links(&stk, &CDinfiniteBB);
    if (!cl)
        return (XIok);

#ifdef TIME_DBG
    Tdbg()->accum_timing("extract_devs");
    Tdbg()->start_timing("connect_to_subs");
#endif
    sSubcGen cg1;
    cg1.set(cl);

    // First, look for connections between celldesc groups and subcell
    // groups.
    ret = connect_to_subs(&cg1);
    if (ret != XIok) {
        sSubcLink::destroy(cl);
        return (ret);
    }
#ifdef TIME_DBG
    Tdbg()->accum_timing("connect_to_subs");
    Tdbg()->start_timing("connect_btwn_subs");
#endif

    // Now look for connections between groups in different subcells.
    ret = connect_between_subs(&cg1);
    if (ret != XIok) {
        sSubcLink::destroy(cl);
        return (ret);
    }
#ifdef TIME_DBG
    Tdbg()->accum_timing("connect_btwn_subs");
    Tdbg()->start_timing("other_stuff");
#endif

    if (EX()->isVerbosePromptline()) {
        PL()->ShowPromptV("Pruning nets in %s...",
            gd_celldesc->cellname()->string());
    }
    if (gd_vcontacts) {
        int cnt = vnextnum();
        alloc_groups(cnt);
        for (sVContact *v = gd_vcontacts; v; v = v->next) {
            if (v->cdesc1) {
                sSubcInst *s1 = add_sc(v->cdesc1, v->ix1, v->iy1);
                s1->add(v->vgroup, v->subg1);
            }
            if (v->cdesc2) {
                sSubcInst *s2 = add_sc(v->cdesc2, v->ix2, v->iy2);
                s2->add(v->vgroup, v->subg2);
            }
        }
        sVContact::destroy(gd_vcontacts);
        gd_vcontacts = 0;
    }

    // Reverse the sSubcInst lists, and add the terminal list elements to
    // the groups.
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        sSubcInst *sn, *s = sl->subs();
        sl->set_subs(0);
        for ( ; s; s = sn) {
            sn = s->next();
            for (sSubcContactInst *c = s->contacts(); c; c = c->next()) {
                if (c->parent_group() >= 0 && c->parent_group() < gd_asize)
                    gd_groups[c->parent_group()].set_subc_contacts(
                        new sSubcContactList(c,
                            gd_groups[c->parent_group()].subc_contacts()));
            }
            s->set_next(sl->subs());
            sl->set_subs(s);
        }
    }

    // If a subckt has two or more references to the same subc_group,
    // merge the two referenced parent_groups, and other things.
    //
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next())
            fixup_subc_contacts(s);
    }

    sSubcLink::destroy(cl);
    renumber_groups();
#ifdef TIME_DBG
    Tdbg()->accum_timing("other_stuff");
    Tdbg()->accum_timing("connect_total");
#endif
    return (XIok);
}


// Add a connection between topgrp in the current level and subgrp
// lower in the hierarchy (described by sSubcGen).  The cells lower in
// the hierarchy have already been grouped, but it may be necessary to
// add virtual "pass-through" contacts.  We try to avoid this by
// checking if the needed subcircuit group is already connected to a
// parent group.
//
int
cGroupDesc::add(int topgrp, const sSubcGen *cg, int subgrp)
{
    int i = 0;
    const sSubcLink *c;
    while ((c = cg->getlink(i)) != 0) {
        CDs *parent = c->cdesc->parent();
        // If somehow an instance copy is returned here, the parent
        // will be null.
        if (!parent)
            continue;
        i++;
        cGroupDesc *gd = parent->groups();
        if (!gd)
            continue;
        if (parent == gd_celldesc) {
            sSubcInst *s = gd->add_sc(c->cdesc, c->ix, c->iy);
            s->add(topgrp, subgrp);
            return (subgrp);
        }

        sSubcInst *s = gd->add_sc(c->cdesc, c->ix, c->iy);
        if (subgrp != 0) {
            // If subgrp is 0, the parent group is also 0, nothing further
            // to do.

            // The parent cell has already been grouped.  First, see if
            // the needed group is already available in the parent.
            sSubcContactInst *sc;
            for (sc = s->contacts(); sc; sc = sc->next()) {
                if (sc->subc_group() == subgrp) {
                    // good, we don't have to add a virtual connection,
                    // just link to the existing parent group
                    subgrp = sc->parent_group();
                    break;
                }
            }
            if (!sc) {
                // Didn't find it, have to create a new virtual group.
                int grp = gd->nextindex();
                gd->alloc_groups(grp+1);
                sc = s->add(grp, subgrp);
                gd->gd_groups[grp].set_subc_contacts(
                    new sSubcContactList(sc,
                    gd->gd_groups[grp].subc_contacts()));
                subgrp = grp;
            }
        }
    }
    return (-1);  // not reached
}


// Add a sSubcInst entry.
//
sSubcInst *
cGroupDesc::add_sc(CDc *cdesc, int ix, int iy)
{
    CDap ap(cdesc);
    for (sSubcList *sl = gd_subckts; sl; sl = sl->next()) {
        if (sl->subs()->cdesc()->master() == cdesc->master()) {
            for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                if (s->cdesc() == cdesc && s->ix() == ix && s->iy() == iy)
                    // found it, already exists
                    return (s);
            }
            int ccnt = cdesc->group() + iy*ap.nx + ix;
            sl->set_subs(new sSubcInst(cdesc, ix, iy, ccnt, sl->subs()));
            return (sl->subs());
        }
    }
    int ccnt = cdesc->group() + iy*ap.nx + ix;
    gd_subckts = new sSubcList(new sSubcInst(cdesc, ix, iy, ccnt, 0), 0,
        gd_subckts);
    return (gd_subckts->subs());
}


// Add a "virtual" contact, this is needed for example when two
// subcircuits abut, forming a contact.
//
int
cGroupDesc::add_vcontact(CDc *c1, int x1, int y1, int n1, CDc *c2, int x2,
    int y2, int n2)
{
    int grp;
    if (!gd_vcontacts)
        grp = nextindex();
    else
        grp = gd_vcontacts->vgroup + 1;
    gd_vcontacts =
        new sVContact(c1, x1, y1, n1, c2, x2, y2, n2, grp, gd_vcontacts);
    return (grp);
}


// Remove duplicate contacts.  Reduce if two or more terminals connect
// to the same subcircuit group.  The number of reductions is returned.
//
int
cGroupDesc::fixup_subc_contacts(sSubcInst *s)
{
    int rcnt = 0;
    for (sSubcContactInst *c = s->contacts(); c; c = c->next()) {
        for (sSubcContactInst *cx = c->next(); cx; cx = cx->next()) {
            if (c->subc_group() == cx->subc_group()) {
                if (c->parent_group() != cx->parent_group()) {
                    reduce(c->parent_group(), cx->parent_group());
                    rcnt++;
                }
            }
        }
    }

    // The subckt may now have duplicate contact entries, purge them.
    //
    for (sSubcContactInst *c = s->contacts(); c; c = c->next()) {
        sSubcContactInst *cp = c, *cn;
        for (sSubcContactInst *cx = c->next(); cx; cx = cn) {
            cn = cx->next();
            if (cx->subc_group() == c->subc_group()) {
                if (cx->parent_group() != c->parent_group()) {
                    ExtErrLog.add_err(
                        "Internal error in fixup_subc_contacts for %s, "
                        "parent groups not equal.",
                        gd_celldesc->cellname()->string());
                }
                else {
                    cp->set_next(cn);
                    bool didit = false;
                    int cpg = cx->parent_group();
                    if (cpg >= 0 && cpg < gd_asize) {
                        sSubcContactList *sp = 0;
                        sSubcContactList *sc = gd_groups[cpg].subc_contacts();
                        for ( ; sc; sc = sc->next()) {
                            if (sc->contact() == cx) {
                                if (!sp)
                                    gd_groups[cpg].set_subc_contacts(
                                        sc->next());
                                else
                                    sp->set_next(sc->next());
                                delete sc;
                                didit = true;
                                break;
                            }
                            sp = sc;
                        }
                    }
                    if (!didit) {
                        ExtErrLog.add_err(
                            "Internal error in fixup_subc_contacts for %s, "
                            "contact not found in list.",
                            gd_celldesc->cellname()->string());
                    }
                    delete cx;
                    continue;
                }
            }
            cp = cx;
        }
    }

    // If the subcircuit contact group is the ground group (0), reflect
    // the parent group to group 0.

    sSubcContactInst *cp = 0, *cn;
    for (sSubcContactInst *c = s->contacts(); c; c = cn) {
        cn = c->next();
        if (c->subc_group() == 0) {
            if (!cp)
                s->set_contacts(cn);
            else
                cp->set_next(cn);
            bool didit = false;
            int cpg = c->parent_group();
            if (cpg >= 0 && cpg < gd_asize) {
                sSubcContactList *sc = gd_groups[cpg].subc_contacts();
                sSubcContactList *sp = 0;
                for ( ; sc; sc = sc->next()) {
                    if (sc->contact() == c) {
                        if (!sp)
                            gd_groups[cpg].set_subc_contacts(sc->next());
                        else
                            sp->set_next(sc->next());
                        delete sc;
                        didit = true;
                        break;
                    }
                    sp = sc;
                }
            }
            if (!didit) {
                ExtErrLog.add_err(
                    "Internal error in fixup_subc_contacts for %s, "
                    "ground contact not found in list.",
                    gd_celldesc->cellname()->string());
            }

            reduce(cpg, 0);
            rcnt++;
            delete c;
            continue;
        }
        cp = c;
    }
    return (rcnt);
}


// The subcircuit has just been renumbered.  Remove contacts that have
// been combined, and update the subc_group of those remaining.
//
void
cGroupDesc::remap_subc_contacts(sSubcInst *s, const int *map)
{
    if (!s || !map)
        return;

    sSubcContactInst *cp = 0, *cn;
    for (sSubcContactInst *c = s->contacts(); c; c = cn) {
        cn = c->next();

        if (map[c->subc_group()] == map[c->subc_group() - 1]) {
            // This indicates that the present subc_group has been
            // unmapped, the the contact should connect to nothing,
            // i.e., be removed.

            if (cp)
                cp->set_next(cn);
            else
                s->set_contacts(cn);
            bool didit = false;
            int cpg = c->parent_group();
            if (cpg >= 0 && cpg < gd_asize) {
                sSubcContactList *sp = 0;
                sSubcContactList *sc = gd_groups[cpg].subc_contacts();
                for ( ; sc; sc = sc->next()) {
                    if (sc->contact() == c) {
                        if (!sp)
                            gd_groups[cpg].set_subc_contacts(sc->next());
                        else
                            sp->set_next(sc->next());
                        delete sc;
                        didit = true;
                        break;
                    }
                    sp = sc;
                }
            }
            if (didit)
                delete c;
            else {
                // If we didn't find the entry, don't delete the
                // contact, as there may be a pointer to it somewhere. 
                // It is unlinked from this list, however.

                ExtErrLog.add_err(
                    "Internal error in remap_subc_contacts for %s, "
                    "contact not found in list.",
                    gd_celldesc->cellname()->string());

                // Remap the subc_group entry.
                c->set_subc_group(map[c->subc_group()]);
            }
            continue;
        }

        // Remap the subc_group entry.
        c->set_subc_group(map[c->subc_group()]);

        cp = c;
    }
}


namespace {

    // A linked list element for integers.
    struct int_list
    {
        int_list(int v, int_list *n)
            {
                next = n;
                value = v;
            }

        void free()
            {
                int_list *l = this;
                while (l) {
                    int_list *x = l;
                    l = l->next;
                    delete x;
                }
            }

        void sort();
        int_list *prune();
        void check_against(int_list*);

        int_list *next;
        int value;
    };

    inline bool icmp(int i1, int i2)
    {
        return (i1 < i2);
    }

    // Sort the list in ascending order of value.
    //
    void
    int_list::sort()
    {
        int_list *l0 = this;
        int cnt = 0;
        for (int_list *l = l0; l; l = l->next, cnt++) ;
        if (cnt < 2)
            return;
        int* ary = new int[cnt];
        cnt = 0;
        for (int_list *l = l0; l; l = l->next)
            ary[cnt++] = l->value;
        std::sort(ary, ary + cnt, icmp);
        cnt = 0;
        for (int_list *l = l0; l; l = l->next)
            l->value = ary[cnt++];
        delete [] ary;
    }

    // Remove any duplicates.  If this leaves only one entry, zero it.
    //
    int_list *
    int_list::prune()
    {
        int_list *ip = this, *in;
        for (int_list *i = ip->next; i; i = in) {
            in = i->next;
            if (ip->value == i->value) {
                ip->next = in;
                delete i;
                continue;
            }
            ip = i;
        }
        if (!next) {
            delete this;
            return (0);
        }
        return (this);
    }

    // Return true if at least one element in *pref is also in list. 
    // If so, prune *pref so that it contains only elements that are
    // also in list, however zero it if there is only one such
    // element.
    //
    bool check_compare(int_list **pref, const int_list *list)
    {
        if (!pref || !*pref || !list)
            return (false);
        int_list *l0 = 0, *le = 0;
        int_list *ref = *pref;
        int_list *ip = 0, *in;
        for (int_list *i = ref; i; i = in) {
            in = i->next;
            bool found = false;
            for (const int_list *j = list; j; j = j->next) {
                if (j->value < i->value)
                    continue;
                if (i->value == j->value)
                    found = true;
                break;
            }
            if (found) {
                if (ip)
                    ip->next = in;
                else
                    ref = in;
                if (!l0)
                    l0 = le = i;
                else {
                    le->next = i;
                    le = i;
                }
                i->next = 0;
                continue;
            }
            ip = i;
        }
        if (l0) {
            ref->free();
            if (!l0->next) {
                delete l0;
                l0 = 0;
            }
            *pref = l0;
            return (true);
        }
        return (false);
    }
}


// Recursive tail for bind_term_group_at_location.  This enables
// binding to a group when the terminal is over a subcircuit, as long
// as it is over a group that can be traced to the top level through
// existing terminals.
//
bool
cGroupDesc::bind_term_group_at_location_rc(const sSubcInstList *sil,
    CDsterm *term, cTfmStack &stk)
{
    CDc *cdesc = sil->inst()->cdesc();
    CDs *msd = cdesc->masterCell();
    if (!msd)
        return (false);
    cGroupDesc *gd = msd->groups();
    if (!gd)
        return (false);
    stk.TPush();
    stk.TApplyTransform(cdesc);
    stk.TPremultiply();
    CDap ap(cdesc);
    stk.TTransMult(sil->inst()->ix()*ap.dx, sil->inst()->iy()*ap.dy);

    int x = term->lx();
    int y = term->ly();
    stk.TInverse();
    stk.TInversePoint(&x, &y);

    for (sSubcContactInst *ci = sil->inst()->contacts(); ci; ci = ci->next()) {
        int topg = sil->top_group(ci->subc_group());
        if (topg < 0)
            continue;

        sGroup &g = gd->gd_groups[ci->subc_group()];
        if (!g.net() || !g.net()->BB().intersect(x, y, true))
            continue;
        for (CDol *ol = g.net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isRouting())
                continue;
            if ((!term->layer() || term->layer() == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(x, y, true)) {
                term->set_v_group(topg);
                term->set_layer(ol->odesc->ldesc());
                return (true);
            }
        }
    }
    if (gd->gd_groups[0].net() &&
            gd->gd_groups[0].net()->BB().intersect(x, y, true)) {
        // Check ground group last.
        for (CDol *ol = gd->gd_groups[0].net()->objlist(); ol; ol = ol->next) {
            if (!ol->odesc->ldesc()->isRouting())
                continue;
            if ((!term->layer() || term->layer() == ol->odesc->ldesc()) &&
                    ol->odesc->intersect(x, y, true)) {
                term->set_v_group(0);
                term->set_layer(ol->odesc->ldesc());
                return (true);
            }
        }
    }
    for (sSubcList *sl = gd->gd_subckts; sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (!s->cdesc()->oBB().intersect(x, y, true))
                continue;

            sSubcInstList *tsl = new sSubcInstList(s, sil);
            bool ret = gd->bind_term_group_at_location_rc(tsl, term, stk);
            delete tsl;
            stk.TPop();
            if (ret)
                return (true);
        }
    }
    return (false);
}


// Bind a terminal to a group (superficially).  This does not handle
// setting the reference object, etc., which should be done before
// this function is called.
//
void
cGroupDesc::bind_term(CDsterm *term, int group)
{
    if (!term)
        return;
    sGroup *g = group_for(group);
    if (g) {
        CDpin *p = new CDpin(term, 0);
        p->set_next(g->termlist());
        g->set_termlist(p);
        g->set_netname(term->name(), sGroup::NameFromTerm);
    }
}


// If the terminal is already bound to a group, unbind it.  This will
// unset the reference object in the terminal.
//
void
cGroupDesc::unbind_term(CDsterm *term)
{
    if (!term)
        return;
    int ogrp = term->group();
    if (ogrp >= 0) {
        term->set_ref(0);

        sGroup *g = group_for(ogrp);
        if (g) {
            CDpin *pp = 0, *pn;
            for (CDpin *p = g->termlist(); p; p = pn) {
                pn = p->next();
                if (p->term() == term) {
                    if (pp)
                        pp->set_next(pn);
                    else
                        g->set_termlist(pn);
                    delete p;
                    break;
                }
                pp = p;
            }
            if (g->netname() == term->name())
                g->set_netname(0, sGroup::NameFromTerm);
        }
    }
}


// Return a list of the cell terminals.
//
CDpin *
cGroupDesc::list_cell_terms(bool fixed_only) const
{
    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    if (!esdesc || esdesc->isEmpty()) {
        // No schematic, but pins may have been created by reading DEF
        // or in some other way.  Return a copy of the list if any are
        // found.

        CDpin *pins = 0, *pe = 0;
        for (CDpin *p = gd_celldesc->pins(); p; p = p->next()) {
            if (!pins)
                pins = pe = new CDpin(p->term(), 0);
            else {
                pe->set_next(new CDpin(p->term(), 0));
                pe = pe->next();
            }
        }
        return (pins);
    }

    CDp_snode **ary;
    unsigned int nsz = esdesc->checkTerminals(&ary);
    if (!nsz)
        return (0);

    CDpin *pins = 0, *pe = 0;
    for (unsigned int i = 0; i < nsz; i++) {
        CDp_snode *ps = ary[i];
        if (!ps || !ps->cell_terminal())
            continue;
        if (fixed_only && !ps->cell_terminal()->is_fixed())
            continue;
        if (!pins)
            pins = pe = new CDpin(ps->cell_terminal(), 0);
        else {
            pe->set_next(new CDpin(ps->cell_terminal(), 0));
            pe = pe->next();
        }
    }
    delete [] ary;
    return (pins);
}


void
cGroupDesc::warn_conductor(int grp, const char *tname)
{
    // Uh-oh, no ROUTING conductor.  Use a CONDUCTOR in this case,
    // but issue warning.
    ExtErrLog.add_err(
        "Warning in %s, terminal %s is associated with a\n"
        "CONDUCTOR layer in group %d since no ROUTING layer was found.",
        gd_celldesc->cellname()->string(), tname, grp);
}


// The recursive tail of fix_connections.
//
void
cGroupDesc::fix_connections_rc(SymTab *done_tab)
{
    done_tab->add((unsigned long)gd_celldesc, 0, false);

    for (sSubcList *s = gd_subckts; s; s = s->next()) {
        CDs *sd = s->subs()->cdesc()->masterCell();
        if (!sd)
            continue;
        if (done_tab->get((unsigned long)sd) != ST_NIL)
            continue;
        cGroupDesc *gd = sd->groups();
        if (!gd)
            continue;
        gd->fix_connections_rc(done_tab);
    }

    itemlist<int_list*> *ref_list = 0;

    sGdList *gdlist = EX()->referenceList(gd_celldesc);
    for (sGdList *gdl = gdlist; gdl; gdl = gdl->next) {
        cGroupDesc *pgd = gdl->gd;

        for (sSubcList *sl = pgd->subckts(); sl; sl = sl->next()) {
            sSubcInst *si = sl->subs();
            if (!si)
                continue;
            if (si->cdesc()->cellname() != gd_celldesc->cellname())
                continue;
            for ( ; si; si = si->next()) {
                int dcnt = 0;
                SymTab tab(false, false);

                for (sSubcContactInst *ci = si->contacts(); ci;
                        ci = ci->next()) {

                    SymTabEnt *ent =
                        tab.get_ent((unsigned long)ci->parent_group());
                    if (ent) {
                        ent->stData = new int_list(ci->subc_group(),
                            (int_list*)ent->stData);
                        dcnt++;
                    }
                    else {
                        tab.add((unsigned long)ci->parent_group(),
                            new int_list(ci->subc_group(), 0), false);
                    }
                }
                if (!dcnt) {
                    SymTabGen gen(&tab, true);
                    SymTabEnt *ent;
                    while ((ent = gen.next()) != 0) {
                        ((int_list*)ent->stData)->free();
                        delete ent;
                    }
                    while (ref_list) {
                        itemlist<int_list*> *ix = ref_list;
                        ref_list = ref_list->next;
                        ix->item->free();
                        delete ix;
                    }
                    return;
                }

                itemlist<int_list*> *list = 0;
                SymTabGen gen(&tab, true);
                SymTabEnt *ent;
                while ((ent = gen.next()) != 0) {
                    int_list *il = (int_list*)ent->stData;
                    il->sort();
                    il = il->prune();
                    if (il)
                        list = new itemlist<int_list*>(il, list);
                    delete ent;
                }
                if (!ref_list)
                    ref_list = list;
                else {
                    itemlist<int_list*> *ip = 0, *in;
                    for (itemlist<int_list*> *i = ref_list; i; i = in) {
                        in = i->next;
                        int_list *ref = i->item;

                        bool found = false;
                        for (itemlist<int_list*> *j = list; j; j = j->next) {
                            if (check_compare(&ref, j->item)) {
                                found = true;
                                break;
                            }
                        }
                        if (!found || !ref) {
                            ref->free();
                            if (ip)
                                ip->next = in;
                            else
                                ref_list = in;
                            delete i;
                            continue;
                        }
                        i->item = ref;
                    }
                    for (itemlist<int_list*> *j = list; j; j = j->next)
                        j->item->free();
                    itemlist<int_list*>::destroy(list);
                    if (!ref_list)
                        return;
                }
            }
            break;
        }
    }

    int ndone = 0;
    for (itemlist<int_list*> *l = ref_list; l; l = l->next) {
        int grp = l->item->value;
        for (int_list *il = l->item->next; il; il = il->next) {
            CDnetName rnm = gd_groups[grp].netname();
            CDnetName nm = gd_groups[il->value].netname();

            // If these exist at this point, they are from labels. 
            // Don't merge conflicting named groups.

            if (rnm && nm && rnm != nm)
                continue;

            ExtErrLog.add_log(ExtLogExt,
                "Merging contact groups %d and %d in %s due to context.",
                grp, il->value, gd_celldesc->cellname()->string());
            reduce(grp, il->value);
            ndone++;
        }
    }
    if (ndone) {

        int *map = 0;
        renumber_groups(&map);

        // The merging of split nets may allow devices to be
        // combined.
        combine_devices();

        for (sGdList *gdl = gdlist; gdl; gdl = gdl->next) {
            cGroupDesc *pgd = gdl->gd;

            for (sSubcList *sl = pgd->subckts(); sl; sl = sl->next()) {
                sSubcInst *si = sl->subs();
                if (!si)
                    continue;
                if (si->cdesc()->cellname() == gd_celldesc->cellname()) {
                    for ( ; si; si = si->next())
                        pgd->remap_subc_contacts(si, map);
                    break;
                }
            }
        }
        for (itemlist<int_list*> *l = ref_list; l; l = l->next) {
            l->item->value = map[l->item->value];
            int cnt = 0;
            for (int_list *il = l->item->next; il; il = il->next, cnt++) ;
            add_lvs_message("  Merged %d %s into group %d due to context.",
                cnt, cnt == 1 ? "group" : "groups", l->item->value);
        }
        add_lvs_message(
            "  The cell appears to have split nets, which are resolved "
            "in the\n"
            "  hierarchy but LVS may fail on the isolated cell.");
        delete [] map;
    }
    for (itemlist<int_list*> *l = ref_list; l; l = l->next)
        l->item->free();
    itemlist<int_list*>::destroy(ref_list);
}


//
// The remaining functions implement flattening of subcells that contain
// wire only or whose names begin with a certain prefix (such as "$$").
// These cells will be invisible in the hierarchy, the contents is reflected
// to the top.
//


namespace {
    inline cGroupDesc *get_gd(sSubcList *sc)
    {
        if (!sc->subs())
            return (0);
        CDc *cdesc = sc->subs()->cdesc();
        if (!cdesc)
            return (0);
        CDs *msdesc = cdesc->masterCell(true);
        if (!msdesc)
            return (0);
        return (msdesc->groups());
    }
}


// Pull any subckts which should be eliminated from the hierarchy into
// the main circuit, recursively.
//
// When called during extraction, the electrical lists are null.  We
// may also call this with assoc true after first-pass association, in
// which case we flatten subcircuits that don't mave a matching
// electrical subcell.
//
bool
cGroupDesc::flatten(bool *changed, bool assoc)
{
    if (changed)
        *changed = false;
    bool done = false;
    bool nogo = false;
    while (!done && !nogo) {
        done = true;

        sSubcList *sp = 0, *sn;
        for (sSubcList *sc = gd_subckts; sc; sc = sn) {
            sn = sc->next();

            if (assoc) {
                bool has_one = false;
                for (sEinstList *e = sc->esubs(); e; e = e->next()) {
                    if (!e->dual_subc()) {
                        has_one = true;
                        break;
                    }
                }
                if (has_one) {
                    sp = sc;
                    continue;
                }
            }

            // Throw out elemets that are bogus (shouldn't be any).
            // Must be done before calling flatten_core.
            cGroupDesc *gd = get_gd(sc);
            if (!gd) {
                if (sp)
                    sp->set_next(sn);
                else
                    gd_subckts = sn;
                delete sc;
                continue;
            }

            int nf, ns;
            if (!flatten_core(sc, assoc, &nf, &ns, 0))
                nogo = true;
            if (!nf) {
                // Nothing flattened.
                sp = sc;
                continue;
            }
            if (changed)
                *changed = true;
            if (!sc->subs()) {
                // All instances were flattened.
                if (sp)
                    sp->set_next(sn);
                else if (!gd_subckts || gd_subckts == sc)
                    gd_subckts = sn;
                else {
                    // Skip over new additions.
                    sp = gd_subckts;
                    while (sp->next() && sp->next() != sc)
                        sp = sp->next();
                    sp->set_next(sn);
                }
                delete sc;
                sc = 0;
            }
            if (ns) {
                // A subckt was added, start over.
                done = false;
                break;
            }
            if (sc)
                sp = sc;
        }
    }

    // Go through the devices again and try to find valid groups for
    // ungrouped contacts.  Ungrouped contacts probably have contact
    // geometry in a subcell, which is now reflected in the main
    // group list.  Also, update the celldesc pointer.
    //
    if (!nogo) {
        for (sDevList *d = gd_devices; d; d = d->next()) {
            for (sDevPrefixList *p = d->prefixes(); p; p = p->next()) {
                for (sDevInst *di = p->devs(); di; di = di->next()) {
                    di->set_celldesc(gd_celldesc);
                    sDevContactInst *ci = di->contacts();
                    for ( ; ci; ci = ci->next()) {
                        if (ci->group() < 0 && ci->desc()->level() != BC_skip)
                            find_contact_group(ci, true);
                    }
                }
            }
        }
    }

    return (!nogo);
}


bool
cGroupDesc::flatten_core(sSubcList *sc, bool assoc, int *nf, int *ns, int *nd)
{
    if (nf)
        *nf = 0;
    if (ns)
        *ns = 0;
    if (nd)
        *nd = 0;
    int fcnt = 0;
    int subc_added = 0;
    int dev_added = 0;
    bool ok = true;

    // We know that these pointers are good, caller checked.
    CDs *md = sc->subs()->cdesc()->masterCell();
    cGroupDesc *gd = md->groups();

    int *map = new int[gd->gd_asize];
    CDcellName mstrname = md->cellname();
    bool isvia = md->isViaSubMaster();

    sSubcInst *sprv = 0, *snxt;
    for (sSubcInst *s = sc->subs(); s; s = snxt) {
        snxt = s->next();
        if (s->dual()) {
            sprv = s;
            continue;
        }
        if (!assoc && !EX()->shouldFlatten(s->cdesc(), gd_celldesc)) {
            sprv = s;
            continue;
        }
        CDol *via = 0;
        if (isvia && !assoc)
            via = new CDol(s->cdesc(), 0);

        fcnt++;
        // Map groups.
        int i;
        for (i = 0; i < gd->gd_asize; i++)
            map[i] = -1;
        for (sSubcContactInst *c = s->contacts(); c; c = c->next())
            map[c->subc_group()] = c->parent_group();
        int count = 0;
        for (i = 1; i < gd->gd_asize; i++) {
            if (map[i] < 0)
                count++;
        }
        int top = nextindex();
        alloc_groups(top + count);
        for (i = 1; i < gd->gd_asize; i++) {
            if (map[i] < 0)
                map[i] = top++;
        }

        // Remove contact references to this subcell.
        for (sSubcContactInst *c = s->contacts(); c; c = c->next())
            remove_contact(c);

        cTfmStack stk;
        stk.TPush();
        stk.TApplyTransform(s->cdesc());
        CDap ap(s->cdesc());
        stk.TTransMult(s->ix()*ap.dx, s->iy()*ap.dy);

        int newgnd = 0;
        if (map[0] < 0)
            map[0] = 0;
        else if (map[0] > 0)
            newgnd = map[0];

        // Yes, probably need group 0 objects, too.
        map[0] = 0;
        for (i = 0; i < gd->gd_asize; i++) {
            int j = map[i];

            sGroup &g = gd->gd_groups[i];

            // Add group objects to extraction phonies and to the net
            // objects list.  If this is a via, record it.  Via
            // objects have the CDmergeDeleted (0x1) flag set.  This
            // choice is rather arbitrary.

            CDol *newnet = 0;
            if (g.net()) {
                for (CDol *o = g.net()->objlist(); o; o = o->next) {
                    CDo *odesc = o->odesc->copyObjectWithXform(&stk);
                    if (!gd_e_phonycell)
                        gd_e_phonycell = new CDs(0, Physical);
                    odesc->set_next_odesc(0);
                    odesc->set_copy(false);
                    odesc->set_flag(CDoMarkExtE);
                    if (!gd_e_phonycell->insert(odesc)) {
                        Errs()->add_error(
                            "Object insertion failed in extraction db.");
                        ok = false;
                    }
                    odesc->set_group(j);
                    newnet = new CDol(odesc, newnet);
                    if (isvia)
                        odesc->set_flag(CDmergeDeleted);
                }
            }
            if (newnet) {
                BBox BB(g.net()->BB());
                stk.TBB(&BB, 0);
                if (!gd_groups[j].net()) {
                    sGroupObjs *go = new sGroupObjs(newnet, &BB);
                    gd_groups[j].set_net(go);
                }
                else {
                    gd_groups[j].net()->BB().add(&BB);
                    CDol *o = newnet;
                    while (o->next)
                        o = o->next;
                    o->next = gd_groups[j].net()->objlist();
                    gd_groups[j].net()->set_objlist(newnet);
                }
                // Note that vias are required to contain some metal.
                if (via) {
                    via->next = gd_groups[j].net()->vialist();
                    gd_groups[j].net()->set_vialist(via);
                    via = 0;
                }
            }
            if (!ok)
                break;
        }
        CDol::destroy(via);  // Should have been used and zeroed.

        // The termlist can be ignored?

        // Add devices and contact references.
        for (sDevList *dv = gd->gd_devices; dv; dv = dv->next()) {
            for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                for (sDevInst *d = p->devs(); d; d = d->next()) {
                    add_dev_copy(&stk, d, map);
                    dev_added++;
                }
            }
        }

        // Add subckts.
        for (sSubcList *sx = gd->gd_subckts; sx; sx = sx->next()) {
            for (sSubcInst *su = sx->subs(); su; su = su->next()) {
                add_subc_copy(&stk, su, map);
                subc_added++;
            }
        }
        if (newgnd > 0)
            reduce(newgnd, 0);

        // templates ?
        // vcontacts ?

        // Record this instance as flattened.  If the instance is
        // arrayed, it is probably in the table already, so check.
        if (!gd_flatten_tab)
            gd_flatten_tab = new SymTab(false, false);
        gd_flatten_tab->add((unsigned long)s->cdesc(), 0, true);

        stk.TPop();

        if (sprv)
            sprv->set_next(snxt);
        else
            sc->set_subs(snxt);
        delete s;

        if (!ok)
            break;
    }
    if (fcnt && mstrname) {
        if (assoc) {
            ExtErrLog.add_log(ExtLogAssoc, "Smashed %d %s of %s into %s.",
                fcnt, fcnt > 1 ? "instances" : "instance",
                mstrname->string(), gd_celldesc->cellname()->string());
        }
        else {
            ExtErrLog.add_log(ExtLogExt, "Smashed %d %s of %s into %s.",
                fcnt, fcnt > 1 ? "instances" : "instance",
                mstrname->string(), gd_celldesc->cellname()->string());
        }
    }
    delete [] map;
    if (nf)
        *nf = fcnt;
    if (ns)
        *ns = subc_added;
    if (nd)
        *nd = dev_added;
    return (ok);
}


namespace {
    struct s_sc { sSubcList *sl; int count; };

    // Sort comparison for subcircuit lists, by instance count.
    inline bool sl_comp_bc(const s_sc &s1, const s_sc &s2)
    {
        return (s1.count < s2.count);
    }


    // Sort comparison for subcircuit lists, alphabetical.
    bool sl_comp(const sSubcList *l1, const sSubcList *l2)
    {
        const char *n1 = 0;
        sSubcInst *s1 = l1->subs();
        if (s1 && s1->cdesc())
            n1 = s1->cdesc()->cellname()->string();
        if (!n1) {
            sEinstList *e1 = l1->esubs();
            if (e1 && e1->cdesc())
                n1 = e1->cdesc()->cellname()->string();
        }
        if (!n1)
            n1 = "";
        const char *n2 = 0;
        sSubcInst *s2 = l2->subs();
        if (s2 && s2->cdesc())
            n2 = s2->cdesc()->cellname()->string();
        if (!n2) {
            sEinstList *e2 = l2->esubs();
            if (e2 && e2->cdesc())
                n2 = e2->cdesc()->cellname()->string();
        }
        if (!n2)
            n2 = "";
        return (strcmp(n1, n2) < 0);
    }
}


// Sort the subcircuit list, in one of two orders.  With bycnt false,
// order is alphabetical, for human convenience in listed output. 
// With bycnt true, sort by instance count, least to most.  This is
// used in association, as it may improve speed to do the "simple"
// cases (fewer instances) first.
//
void
cGroupDesc::sort_subs(bool bycnt)
{
    int cnt = 0;
    for (sSubcList *s = gd_subckts; s; s = s->next())
        cnt++;
    if (cnt < 2)
        return;
    if (bycnt) {
        s_sc *ary = new s_sc[cnt];
        cnt = 0;
        for (sSubcList *s = gd_subckts; s; s = s->next()) {
            int scnt = 0;
            for (sSubcInst *si = s->subs(); si; si = si->next())
                scnt++;
            ary[cnt].sl = s;
            ary[cnt].count = scnt;
            cnt++;
        }
        std::sort(ary, ary + cnt, sl_comp_bc);
        for (int i = 1; i < cnt; i++)
            ary[i-1].sl->set_next(ary[i].sl);
        ary[cnt-1].sl->set_next(0);
        gd_subckts = ary[0].sl;
        delete [] ary;
    }
    else {
        sSubcList **ary = new sSubcList*[cnt];
        cnt = 0;
        for (sSubcList *sc = gd_subckts; sc; sc = sc->next())
            ary[cnt++] = sc;
        std::sort(ary, ary + cnt, sl_comp);
        for (int i = 1; i < cnt; i++)
            ary[i-1]->set_next(ary[i]);
        ary[cnt-1]->set_next(0);
        gd_subckts = ary[0];
        delete [] ary;
    }
}


namespace {
    // Sort comparison function for subcircuits, spatial or array index.
    bool si_comp(const sSubcInst *s1, const sSubcInst *s2)
    {
        if (s1->cdesc() == s2->cdesc()) {
            CDap ap(s1->cdesc());
            int n1 = s1->iy()*ap.nx + s1->ix();
            int n2 = s2->iy()*ap.nx + s2->ix();
            return (n1 < n2);
        }
        if (s1->cdesc()->oBB().top > s2->cdesc()->oBB().top)
            return (true);
        if (s1->cdesc()->oBB().top < s2->cdesc()->oBB().top)
            return (false);
        return (s1->cdesc()->oBB().left < s2->cdesc()->oBB().left);
    }
}


// Sort the instances into database order:  descending in top y,
// ascending in x for same top y.  Not sure if this is really
// necessary, but it provides well-defined order.
//
void
cGroupDesc::sort_and_renumber_subs()
{
    // Sort the subcircuit masters alphabetically.
    sort_subs(false);

    int id = 0;
    for (sSubcList *sc = gd_subckts; sc; sc = sc->next()) {

        int cnt = 0;
        for (sSubcInst *su = sc->subs(); su; su = su->next())
            cnt++;
        sSubcInst **ary = new sSubcInst*[cnt];
        cnt = 0;
        for (sSubcInst *su = sc->subs(); su; su = su->next())
            ary[cnt++] = su;
        std::sort(ary, ary + cnt, si_comp);
        for (int i = 1; i < cnt; i++)
            ary[i-1]->set_next(ary[i]);
        ary[cnt-1]->set_next(0);
        sc->set_subs(ary[0]);
        delete [] ary;

        // Renumber.
        int ix = 0;
        for (sSubcInst *su = sc->subs(); su; su = su->next()) {
            su->set_uid(id++);
            su->set_index(ix++);
        }
    }
}


namespace {
    struct s_dc { sDevList *dl; int count; };

    // Sort comparison function for devices, by instance count.
    inline bool dl_comp_bc(const s_dc &d1, const s_dc &d2)
    {
        return (d1.count < d2.count);
    }


    // Sort comparison function for devices, alphabetical by name.
    bool dl_comp(const sDevList *d1, const sDevList *d2)
    {
        const char *n1 = d1->devname()->string();
        if (!n1)
            n1 = "";
        const char *n2 = d2->devname()->string();
        if (!n2)
            n2 = "";
        return (strcmp(n1, n2) < 0);
    }


    // Sort comparison function for devices, alphabetical by prefix.
    bool dp_comp(const sDevPrefixList *p1, const sDevPrefixList *p2)
    {
        const char *n1 = 0;
        sDevInst *d1 = p1->devs();
        if (d1)
            n1 = d1->desc()->prefix();
        if (!n1)
            n1 = "";
        const char *n2 = 0;
        sDevInst *d2 = p2->devs();
        if (d2)
            n2 = d2->desc()->prefix();
        if (!n2)
            n2 = "";
        return (strcmp(n1, n2) < 0);
    }
}


// Sort the subcircuit list, in one of two orders.  With bycnt false,
// order is alphabetical, for human convenience in listed output. 
// With bycnt true, sort by instance count, least to most.  This is
// used in association, as it may improve speed to do the "simple"
// cases (fewer instances) first.
//
void
cGroupDesc::sort_devs(bool bycnt)
{
    int cnt = 0;
    for (sDevList *d = gd_devices; d; d = d->next())
        cnt++;
    if (cnt < 2)
        return;
    if (bycnt) {
        s_dc *ary = new s_dc[cnt];
        cnt = 0;
        for (sDevList *d = gd_devices; d; d = d->next()) {
            int dcnt = 0;
            for (sDevPrefixList *p = d->prefixes(); p; p = p->next()) {
                for (sDevInst *di = p->devs(); di; di = di->next())
                    dcnt++;
            }
            ary[cnt].dl = d;
            ary[cnt].count = dcnt;
            cnt++;
        }
        std::sort(ary, ary + cnt, dl_comp_bc);
        for (int i = 1; i < cnt; i++)
            ary[i-1].dl->set_next(ary[i].dl);
        ary[cnt-1].dl->set_next(0);
        gd_devices = ary[0].dl;
        delete [] ary;
    }
    else {
        sDevList **ary = new sDevList*[cnt];
        cnt = 0;
        for (sDevList *d = gd_devices; d; d = d->next())
            ary[cnt++] = d;
        std::sort(ary, ary + cnt, dl_comp);
        for (int i = 1; i < cnt; i++)
            ary[i-1]->set_next(ary[i]);
        ary[cnt-1]->set_next(0);
        gd_devices = ary[0];
        delete [] ary;

        // Sort the prefixes alphabetically as well.
        for (sDevList *d = gd_devices; d; d = d->next()) {
            cnt = 0;
            for (sDevPrefixList *p = d->prefixes(); p; p = p->next())
                cnt++;
            if (cnt > 1) {
                sDevPrefixList **pary = new sDevPrefixList*[cnt];
                cnt = 0;
                for (sDevPrefixList *p = d->prefixes(); p; p = p->next())
                    pary[cnt++] = p;
                std::sort(pary, pary + cnt, dp_comp);
                for (int i = 1; i < cnt; i++)
                    pary[i-1]->set_next(pary[i]);
                pary[cnt-1]->set_next(0);
                d->set_prefixes(pary[0]);
                delete [] pary;
            }
        }
    }
}


namespace {
    // Sort comparison function for devices, spatial ordering.
    bool dv_comp(const sDevInst *d1, const sDevInst *d2)
    {
        if (d1->BB()->top > d2->BB()->top)
            return (true);
        if (d1->BB()->top < d2->BB()->top)
            return (false);
        return (d1->BB()->left < d2->BB()->left);
    }
}


// Sort and renumber the devices.  We sort into database order of the
// body bounding box.  Not sure this is needed, but it establishes a
// well-defined order.
//
void
cGroupDesc::sort_and_renumber_devs()
{
    // Sort the devices and prefixes alphabetically.
    sort_devs(false);

    for (sDevList *d = gd_devices; d; d = d->next()) {
        for (sDevPrefixList *p = d->prefixes(); p; p = p->next()) {
            int cnt = 0;
            for (sDevInst *di = p->devs(); di; di = di->next())
                cnt++;
            sDevInst **ary = new sDevInst*[cnt];
            cnt = 0;
            for (sDevInst *di = p->devs(); di; di = di->next())
                ary[cnt++] = di;
            std::sort(ary, ary + cnt, dv_comp);
            for (int i = 1; i < cnt; i++) {
                int j = i-1;
                ary[j]->set_next(ary[i]);
                ary[j]->set_index(j);
            }
            cnt--;
            ary[cnt]->set_next(0);
            ary[cnt]->set_index(cnt);
            p->set_devs(ary[0]);
            delete [] ary;
        }
    }
}


// Return true if the cell contains a subcircuit or more than one
// device.  Devices inherited from flattening the cell will be
// prevented from being merged, since the schematic representation
// will not show such a merge.
//
bool
cGroupDesc::check_merge()
{
    if (gd_subckts)
        return (true);
    if (gd_celldesc->isPCellSubMaster())
        return (false);
    int cnt = 0;
    for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
        for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
            for (sDevInst *d = p->devs(); d; d = d->next()) {
                if (cnt)
                    return (true);
                cnt++;
            }
        }
    }
    return (false);
}


// Remove the contact from the group array.
//
void
cGroupDesc::remove_contact(sSubcContactInst *c)
{
    int grp = c->parent_group();
    if (grp < 0 || grp >= gd_asize)
        return;
    sSubcContactList *sp = 0, *sn;
    for (sSubcContactList *s = gd_groups[grp].subc_contacts(); s; s = sn) {
        sn = s->next();
        if (s->contact() == c) {
            if (!sp)
                gd_groups[grp].set_subc_contacts(sn);
            else
                sp->set_next(sn);
            delete s;
            return;
        }
        sp = s;
    }
}


// Add a transformed copy of the device, a pointer to which is
// returned.
//
sDevInst *
cGroupDesc::add_dev_copy(cTfmStack *tstk, sDevInst *di, int *map)
{
    di = di->copy(tstk, map);

    // This keeps track of the flattening depth, have to use this
    // when using, e.g., body.getZlist().
    di->set_bdepth(di->bdepth() + 1);

    sDevList *dl;
    sDevPrefixList *p = 0;
    for (dl = gd_devices; dl; dl = dl->next()) {
        for (p = dl->prefixes(); p; p = p->next()) {
            if (p->devs()->desc() == di->desc())
                break;
        }
        if (p)
            break;
    }
    int ind = 0;
    if (!dl)
        gd_devices = new sDevList(di, gd_devices);
    else {
        sDevInst *dp = 0;
        for (sDevInst *d = p->devs(); d; dp = d, d = d->next())
            if (d->index() > ind)
                ind = d->index();
        ind++;
        dp->set_next(di);
    }
    di->set_index(ind);

    // add the contact references
    for (sDevContactInst *c = di->contacts(); c; c = c->next()) {
        if (c->desc()->level() == BC_defer && c->group() < 0) {
            // See if we can resolve the contact in the new parent.

            XIrt xrt;
            sDevContactInst *cx = di->desc()->identify_bulk_contact(
                gd_celldesc, di, c->desc(), &xrt);
            if (cx) {
                c->set_BB(cx->cBB());
                c->set_fillfct(cx->fillfct());
                delete cx;
                if (find_contact_group(c, true) >= 0)
                    return (di);
            }
        }
        if (!link_contact(c) && c->desc()->level() == BC_immed) {
            ExtErrLog.add_err(
                "In %s, add_dev_copy: %s %d %s index out of range %d %d.",
                gd_celldesc->cellname()->string(),
                di->desc()->name()->string(), di->index(),
                c->desc()->name()->string(), c->group(), gd_asize);
        }
    }
    return (di);
}


// Add a transformed copy of the subckt.
//
void
cGroupDesc::add_subc_copy(cTfmStack *tstk, sSubcInst *su, int *map)
{
    su = su->copy(copy_cdesc(tstk, su->cdesc()), map);

    CDs *tsd = su->cdesc()->masterCell(true);
    sSubcList *sl;
    for (sl = gd_subckts; sl; sl = sl->next()) {
        if (sl->subs()->cdesc()->masterCell(true) == tsd)
            break;
    }
    int ind = 0;
    if (!sl)
        gd_subckts = new sSubcList(su, 0, gd_subckts);
    else {
        sSubcInst *sp = 0;
        for (sSubcInst *s = sl->subs(); s; sp = s, s = s->next()) {
            if (s->uid() > ind)
                ind = s->uid();
        }
        ind++;
        sp->set_next(su);
    }
    su->set_uid(ind);

    // add the contact references
    for (sSubcContactInst *c = su->contacts(); c; c = c->next()) {
        int cpg = c->parent_group();
        if (cpg >= 0 && cpg < gd_asize)
            gd_groups[cpg].set_subc_contacts(
                new sSubcContactList(c, gd_groups[cpg].subc_contacts()));
        else {
            ExtErrLog.add_err(
                "In %s, add_subc_copy: index out of range %d %d.",
                gd_celldesc->cellname()->string(), cpg, gd_asize);
        }
    }
}


// Copy the cdesc, supplying a new transform and CDm struct.  This is
// called when it is necessary to push a subcell up to the parent due
// to a flatten.  The transform of the containing cdesc is pushed.
//
CDc *
cGroupDesc::copy_cdesc(cTfmStack *tstk, CDc *cdesc)
{
    BBox nBB;
    CDs *sd = cdesc->masterCell();
    if (sd) {  
        const BBox *BB = sd->BB();
        if (BB->left != CDinfinity)
            nBB = *BB;    
    }
    CDap ap(cdesc);
    if (ap.nx > 1) {
        if (ap.dx > 0)
            nBB.right += (ap.nx - 1)*ap.dx;
        else
            nBB.left += (ap.nx - 1)*ap.dx;
    }
    if (ap.ny > 1) {
        if (ap.dy > 0)
            nBB.top += (ap.ny - 1)*ap.dy;
        else
            nBB.bottom += (ap.ny - 1)*ap.dy;
    }

    CDc *cd = new CDc(cdesc->ldesc());
    cd->set_copy(true);
    tstk->TPush();
    tstk->TApplyTransform(cdesc);
    tstk->TPremultiply();
    tstk->TBB(&nBB, 0);
    CDtf tf;
    tstk->TCurrent(&tf);
    tstk->TPop();
    cd->setTransform(&tf, &ap);
    cd->set_oBB(nBB);

    for (CDm *m = gd_master_list; m; m = (CDm*)m->tab_next()) {
        if (m->cellname() == cdesc->cellname() &&
                m->parent() == gd_celldesc) {
            cd->setMaster(m);
            cd->linkIntoMaster();
            return (cd);
        }
    }
    CDm *m = new CDm(cdesc->cellname());
    m->setCelldesc(cdesc->masterCell(true));
    m->setParent(gd_celldesc);
    m->set_tab_next(gd_master_list);
    gd_master_list = m;
    cd->setMaster(m);
    cd->linkIntoMaster();
    return (cd);
}
// End of cGroupDesc functions.


// Return true if the contact connects to metal only.
//
bool
sSubcContactInst::is_wire_only() const
{
    CDs *ssd = subc()->cdesc()->masterCell(true);
    cGroupDesc *sgd = ssd ? ssd->groups() : 0;
    if (!sgd)
        return (false);
    sGroup *sg = sgd->group_for(subc_group());
    if (!sg)
        return (false);
    if (sg->device_contacts())
        return (false);
    if (sg->unas_wire_only())
        return (true);
    for (sSubcContactList *cl = sg->subc_contacts(); cl; cl = cl->next()) {
        if (!cl->contact()->is_wire_only())
            return (false);
    }
    return (true);
}


bool
sSubcContactInst::is_global() const
{
    CDs *ssd = subc()->cdesc()->masterCell(true);
    cGroupDesc *sgd = ssd ? ssd->groups() : 0;
    if (!sgd)
        return (false);
    sGroup *sg = sgd->group_for(subc_group());
    if (!sg)
        return (false);
    return (sg->global() && sg->netname());
}
// End of sSubcContactInst functions.


sSubcInst::sSubcInst(sVContact *v, int which, sSubcInst *n)
{
    sc_next = n;
    if (which == 1) {
        sc_cdesc = v->cdesc1;
        sc_ix = v->ix1;
        sc_iy = v->iy1;
        CDap ap(v->cdesc1);
        sc_uid = v->cdesc1->group() + v->iy1*ap.nx + v->ix1;
    }
    else {
        sc_cdesc = v->cdesc2;
        sc_ix = v->ix2;
        sc_iy = v->iy2;
        CDap ap(v->cdesc2);
        sc_uid = v->cdesc2->group() + v->iy2*ap.nx + v->ix2;
    }
    sc_contacts = 0;
    sc_glob_conts = 0;
    sc_dual = 0;
    sc_index = 0;
}


// Add a contact to a subcircuit, between group parent and group sub
// in the subcell, and return it.
//
sSubcContactInst *
sSubcInst::add(int parent, int sub)
{
    for (sSubcContactInst *c = sc_contacts; c; c = c->next()) {
        if (c->parent_group() == parent && c->subc_group() == sub)
            return (c);
    }
    sc_contacts = new sSubcContactInst(parent, sub, this, sc_contacts);
    return (sc_contacts);
}


// Copy function for sSubcInst structs.  Use the group mapping if given.
//
sSubcInst *
sSubcInst::copy(CDc *cd, int *map)
{
    sSubcInst *s = new sSubcInst(cd ? cd : sc_cdesc, sc_ix, sc_iy, sc_uid, 0);
    sSubcContactInst *end = 0;
    for (sSubcContactInst *c = sc_contacts; c; c = c->next()) {
        if (!end) {
            s->sc_contacts = end = new sSubcContactInst(
                map ? map[c->parent_group()] : c->parent_group(),
                c->subc_group(), s, 0);
        }
        else {
            end->set_next(new sSubcContactInst(
                map ? map[c->parent_group()] : c->parent_group(),
                c->subc_group(), s, 0));
            end = end->next();
        }
    }
    return (s);
}


// This creates subcircuit templates and consistently orders instance
// connections, called after extraction.
//
void
sSubcInst::update_template()
{
    sSubcDesc *sd = EX()->findSubcircuit(cdesc()->masterCell());
    if (!sd) {
        sd = new sSubcDesc(this);
        EX()->addSubcircuit(sd);
    }
    else
        sd->sort_and_update(this);
}


// Return a concocted instance name consisting of the master name,
// followed by an underscore, then the index number.
//
char *
sSubcInst::instance_name()
{
    char buf[16];
    sprintf(buf, "%d", sc_index);
    const char *nm = sc_cdesc ? sc_cdesc->cellname()->string() : 0;
    if (!nm)
        nm = "UNKNOWN";
    char *name = new char[strlen(nm) + strlen(buf) + 2];
    sprintf(name, "%s_%s", nm, buf);
    return (name);
}
// End of sSubcInst functions.


sSubcDesc::sSubcDesc(const sSubcInst *s)
{
    // The s, which is the first encountered, sets the contact order. 
    // One should call sort_and_update for every other instance.

    sd_sdescF = (unsigned long)s->cdesc()->masterCell();

    int numcontacts = 0;
    for (sSubcContactInst *c = s->contacts(); c; c = c->next())
        numcontacts++;

    int *a = new int[numcontacts + 2];
    a[0] = numcontacts;
    int i = 1;
    for (sSubcContactInst *c = s->contacts(); c; c = c->next())
        a[i++] = c->subc_group();
    a[i] = 0;
    sd_array = a;
}


// Sort the contacts in the subcircuit connections.  Additional contacts
// are added to the template as found.
//
void
sSubcDesc::sort_and_update(sSubcInst *s)
{
    unsigned int asz = 0;
    sSubcContactInst *c;
    for (c = s->contacts(); c; c = c->next())
        asz++;

    sSubcContactInst **ary = new sSubcContactInst*[asz];
    asz = 0;
    for (c = s->contacts(); c; c = c->next())
        ary[asz++] = c;

    sSubcContactInst *end = 0;
    s->set_contacts(0);
    unsigned int numcontacts = num_contacts();
    for (unsigned j = 0; j < numcontacts; j++) {
        for (unsigned int k = 0; k < asz; k++) {
            if (ary[k] && ary[k]->subc_group() == contact(j)) {
                if (!end) {
                    end = ary[k];
                    s->set_contacts(end);
                }
                else {
                    end->set_next(ary[k]);
                    end = end->next();
                }
                end->set_next(0);
                ary[k] = 0;
                break;
            }
        }
    }

    int numnew = 0;
    for (unsigned int k = 0; k < asz; k++) {
        if (ary[k])
            numnew++;
    }
    if (numnew > 0) {
        // Any contacts remaining are new, add them to the template.

        unsigned int osz = group_end();
        int *nary = new int[osz + numnew];
        nary[0] = numcontacts + numnew;
        unsigned int i = 1;
        for ( ; i <= numcontacts; i++)
            nary[i] = sd_array[i];

        for (unsigned int k = 0; k < asz; k++) {
            if (ary[k]) {
                if (!end) {
                    end = ary[k];
                    s->set_contacts(end);
                }
                else {
                    end->set_next(ary[k]);
                    end = end->next();
                }
                end->set_next(0);
                nary[i++] = end->subc_group();
                ary[k] = 0;
            }
        }
        memcpy(nary + i, sd_array + numcontacts + 1,
            (osz - (numcontacts + 1))*sizeof(int));
        delete [] sd_array;
        sd_array = nary;
    }
    delete [] ary;
}
// End of sSubcDesc functions.

