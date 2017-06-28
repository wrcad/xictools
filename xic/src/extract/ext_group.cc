
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
 $Id: ext_group.cc,v 5.183 2017/03/14 01:26:38 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_nets.h"
#include "ext_grpgen.h"
#include "ext_errlog.h"
#include "ext_ufb.h"
#include "sced.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "cd_netname.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "tech.h"
#include "events.h"
#include "errorlog.h"
#include <algorithm>

#define TIME_DBG
#ifdef TIME_DBG
#include "timedbg.h"
#endif


//========================================================================
//
// Functions for "grouping".  This gives a common group number to all
// electrically connected conductor objects in the physical cell. 
// This understands connections by vias and contact layers.  This is
// the initial operation when extracting.
//
//========================================================================


// Debugging aid.
//
void
DumpAll(CDs *sdesc, char *fname)
{
    FILE *fp = fopen(fname, "w");
    if (fp) {
        CDgenHierDn_s gen(sdesc);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0) {
            cGroupDesc *gd = sd->groups();
            if (gd)
                gd->dump(fp);
        }
        fclose(fp);
    }
}


// Assign group numbers to the wire nets in sdesc to depth.
// If false is returned, grouping was aborted.
//
bool
cExt::group(CDs *sdesc, int depth)
{
    if (!sdesc)
        return (true);
    if (sdesc->isElectrical())
        return (false);

    // Init the GlobalExclude sLspec.
    const char *gexpr = CDvdb()->getVariable(VA_GlobalExclude);
    if (!gexpr || !*gexpr)
        ext_global_excl.reset();
    else if (!ext_global_excl.parseExpr(&gexpr)) {
        Errs()->add_error("GlobalExclude expression parse failed");
        Errs()->add_error("Grouping failed");
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return (false);
    }
    if (!ext_global_excl.setup()) {
        Errs()->add_error("GlobalExclude expression initialization failed");
        Errs()->add_error("Grouping failed");
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return (false);
    }

    CDextLgen::set_dirty();  // Refresh our layer generators.

    cGroupDesc *gd = sdesc->groups();
    if (!gd) {
        gd = new cGroupDesc(sdesc);
        sdesc->setGroups(gd);
    }

    XIrt ret = XIok;
    // Note: cellnames are all in name table.
    bool conn = sdesc->isConnected();
    bool gpinv = sdesc->isGPinv();
    if (!conn || !gpinv) {
        // If the grouping is invalid for this, invalidate the grouping
        // for all cells lower in the hierarchy.  The grouping depends
        // on cells higher in the hierarchy.  Ditto for the ground
        // plane.
        CDgenHierDn_s gen(sdesc);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0) {
            if (!conn)
                sd->setConnected(false);
            if (!gpinv)
                sd->setGPinv(false);
        }
        if (err)
            return (false);
    }

    dspPkgIf()->SetWorking(true);
    if (EX()->isVerbosePromptline())
        PL()->PushPrompt("Grouping: ");
    else
        PL()->ShowPrompt("Grouping ...");

    ExtErrLog.start_logging(ExtLogGrp, sdesc->cellname());

    if (!CDextLgen::ext_ltab()->num_conductors()) {
        ExtErrLog.add_err(
"Warning: technology contains no CONDUCTOR layers.  Extraction will\n"
"fail without any CONDUCTORS defined.");
    }
    if (!CDextLgen::ext_ltab()->num_routing()) {
        ExtErrLog.add_err(
"Warning: technology contains no ROUTING layers.  Conductors connecting\n"
"subcells must have the ROUTING attribute.  Extraction will generally fail\n"
"without ROUTING layers.");
    }

    // Mark this cell as top-level while grouping.
    gd->set_top_level(true);
    ret = EX()->setupGroundPlane(sdesc);
    if (ret == XIok) {
#ifdef TIME_DBG
        Tdbg()->start_timing("grouping");
#endif
        SymTab tab(false, false);
        ret = group_rec(sdesc, depth, &tab);
#ifdef TIME_DBG
        Tdbg()->accum_timing("grouping");
        Tdbg()->print_accum("grouping");
#endif
    }
    gd->set_top_level(false);

    ExtErrLog.add_log(ExtLogGrp, "Grouping %s complete, %s.",
        sdesc->cellname()->string(),
        ret == XIok ? "no errors" : "error(s) encountered");
    ExtErrLog.end_logging();

    if (EX()->isVerbosePromptline())
        PL()->PopPrompt();
    if (ret == XIok) {
        if (EX()->isVerbosePromptline())
            PL()->ShowPromptV("Grouping complete in %s.",
                sdesc->cellname()->string());
    }
    else
        PL()->ShowPrompt("Grouping aborted.");
    dspPkgIf()->SetWorking(false);

    return (ret == XIok);
}


// Call this if something global changes which invalidates the groups. 
// Clear all groups, and of course extraction/association.  It is
// possible to revert extraction separately (just unset the flags)
// however grouping contributes little overhead in comparison with
// extraction and association, so it is safer to just start from
// scratch.
//
// The inverted ground plane is kept unless true is passed, in which
// case it is cleared.
//
void
cExt::invalidateGroups(bool gptoo)
{
    if (DSP()->CurCellName())
        EV()->InitCallback();

    CDgenTab_s sgen(Physical);
    CDs *sd;
    while ((sd = sgen.next()) != 0)
        sd->groups()->clear_groups(gptoo);
    const char *tabname = CDcdb()->tableName();
    CDcdb()->rotateTable();
    while (CDcdb()->tableName() != tabname) {
        sgen = CDgenTab_s(Physical);
        while ((sd = sgen.next()) != 0)
            sd->groups()->clear_groups(gptoo);
    }

    // Blow away all the sSubcDescs and table.
    SymTabGen gen(ext_subckt_tab, true);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        delete (sSubcDesc*)ent->stData;
        delete ent;
    }
    delete ext_subckt_tab;
    ext_subckt_tab = 0;
}


// Destroy the groups info struct, zero pointer in sdesc, for export.
//
void
cExt::destroyGroups(CDs *sd)
{
    if (!sd || sd->isElectrical())
        return;
    cGroupDesc *gd = sd->groups();
    gd->clear_groups();
    sd->setGroups(0);
    delete gd;
}


// Clear the group descriptor, for export.
//
void
cExt::clearGroups(CDs *sd)
{
    if (!sd || sd->isElectrical())
        return;
    cGroupDesc *gd = sd->groups();
    gd->clear_groups();
}


// Private recursive core for the group function.
//
XIrt
cExt::group_rec(CDs *sdesc, int depth, SymTab *tab)
{
    tab->add((unsigned long)sdesc, 0, false);

    XIrt ret = XIok;
    if (depth > 0) {
        CDm_gen mgen(sdesc, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
            CDs *msdesc = md->celldesc();
            if (!msdesc)
                continue;
            if (tab->get((unsigned long)msdesc) != ST_NIL)
                continue;
            ret = group_rec(msdesc, depth - 1, tab);
            if (ret != XIok)
                break;
        }
    }
    if (ret == XIok) {
        if (!sdesc->isConnected()) {
            cGroupDesc *gd = sdesc->groups();
            if (!gd) {
                gd = new cGroupDesc(sdesc);
                sdesc->setGroups(gd);
            }
            activateGroundPlane(true);
            ret = gd->setup_groups();
            activateGroundPlane(false);
        }
        if (sdesc->cellname() == DSP()->CurCellName() && isShowingGroups()) {
            cGroupDesc *gd = sdesc->groups();
            if (gd) {
                gd->set_group_display(true);
                WindowDesc *wd;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                while ((wd = wgen.next()) != 0)
                    gd->show_groups(wd, DISPLAY);
            }
        }
        if (ret == XIok) {
            if (EX()->isVerbosePromptline())
                PL()->ShowPromptV("Grouping complete in %s.",
                    sdesc->cellname()->string());
        }
        else
            PL()->ShowPrompt("Grouping aborted.");
    }
    return (ret);
}
// End of cExt functions.


// This is the main function for establishing the conductor grouping.
//
XIrt
cGroupDesc::setup_groups()
{
    clear_groups();

    // Setup table of cell instances to henceforth ignore.
    find_ignored();

    // Process objects on layers with an exclude directive.
    if (!process_exclude())
        return (XIbad);

    // do the grouping
    XIrt ret = group_objects();
    if (ret != XIok) {
        clear_groups();
        return (ret);
    }
    gd_celldesc->setConnected(true);
    return (XIok);
}


// Destroy the groups and the lists in grdesc, but not grdesc itself. 
// The extraction and duality are gone, too.  Keep the inverted ground
// plane, if any, unless true is passed.
//
void
cGroupDesc::clear_groups(bool gptoo)
{
    {
        cGroupDesc *gdt = this;
        if (!gdt)
            return;
    }

    clear_extract();

    SI()->ClearGroups(this);
    if (gd_celldesc == CurCell(Physical)) {
        EX()->clearDeviceSelection();
        if (EX()->isShowingGroups()) {
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0)
                show_groups(wdesc, ERASE);
            set_group_display(false);
        }
        if (EX()->isShowingDevs()) {
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0)
                show_devs(wdesc, ERASE);
        }
        EX()->setShowingGroups(false);
        EX()->setShowingNodes(false);
        EX()->PopUpExtSetup(0, MODE_UPD);
    }
    delete [] gd_groups;
    gd_groups = 0;
    gd_asize = 0;
    delete gd_g_phonycell;
    gd_g_phonycell = 0;
    gd_devices->free();
    gd_devices = 0;
    delete gd_ignore_tab;
    gd_ignore_tab = 0;
    set_top_level(false);
    if (gptoo && EX()->groundPlaneLayerInv()) {
        gd_celldesc->setGPinv(false);
        gd_celldesc->db_clear_layer(EX()->groundPlaneLayerInv());
    }
    gd_celldesc->setConnected(false);
}


// If AOI touches an object in the phony list, return the object desc. 
// Export for group.node selection.  The layer must be visible and
// selectable.
//
CDo *
cGroupDesc::intersect_phony(BBox *AOI)
{
    CDlgen gen(Physical, CDlgen::TopToBotNoCells);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (ld->isInvisible())
            continue;
        if (ld->isNoSelect())
            continue;
        if (gd_g_phonycell) {
            CDg gdesc;
            gdesc.init_gen(gd_g_phonycell, ld, AOI);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->intersect(AOI, true))
                    return (odesc);
            }
        }
        if (gd_e_phonycell) {
            CDg gdesc;
            gdesc.init_gen(gd_e_phonycell, ld, AOI);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->intersect(AOI, true))
                    return (odesc);
            }
        }
    }
    return (0);
}


// For debugging.
//
void
cGroupDesc::dump(FILE *fp)
{
    fprintf(fp, "Cell %s summary\n", gd_celldesc->cellname()->string());
    fprintf(fp, "Allocated groups: %d\n", gd_asize);

    fprintf(fp, "Grouping temporary objects:\n");
    {
        CDsLgen lgen(gd_g_phonycell);
        lgen.sort();
        CDl *ld;
        unsigned int tot = 0;
        while ((ld = lgen.next()) != 0) {
            CDtree *t = gd_g_phonycell->db_find_layer_head(ld);
            if (!t)
                continue;
            unsigned int n = t->num_elements();
            if (n > 0)
                fprintf(fp, "  %s %u\n", ld->name(), n);
            tot += n;
        }
        fprintf(fp, "  total %u\n", tot);
    }

    fprintf(fp, "Extraction temporary objects:\n");
    {
        CDsLgen lgen(gd_e_phonycell);
        lgen.sort();
        CDl *ld;
        unsigned int tot = 0;
        while ((ld = lgen.next()) != 0) {
            CDtree *t = gd_e_phonycell->db_find_layer_head(ld);
            if (!t)
                continue;
            unsigned int n = t->num_elements();
            if (n > 0)
                fprintf(fp, "  %s %u\n", ld->name(), n);
            tot += n;
        }
        fprintf(fp, "  total %u\n", tot);
    }

    if (gd_etlist) {
        fprintf(fp, "Electrical connections:\n");
        gd_etlist->dump(fp);
    }

    if (gd_subckts) {
        fprintf(fp, "Subcircuit instances:\n");
        for (sSubcList *s = gd_subckts; s; s = s->next()) {
            int cnt = 0;
            int cpycnt = 0;
            for (sSubcInst *su = s->subs(); su; su = su->next()) {
                cnt++;
                if (su->iscopy())
                    cpycnt++;
            }
            if (cnt) {
                fprintf(fp, "  %-16s %-5d copies %-5d\n",
                    s->subs()->cdesc()->cellname()->string(),
                    cnt, cpycnt);
            }
        }
    }
    if (gd_extra_subs) {
        int cnt = 0;
        for (sEinstList *s = gd_extra_subs; s; s = s->next(), cnt++) ;
        fprintf(fp, "Extra subcircuits: %d\n", cnt);
    }

    if (gd_devices) {
        fprintf(fp, "Device instances:\n");
        for (sDevList *dv = gd_devices; dv; dv = dv->next()) {
            for (sDevPrefixList *p = dv->prefixes(); p; p = p->next()) {
                int cnt = 0;
                for (sDevInst *di = p->devs(); di; di = di->next())
                    cnt++;
                fprintf(fp, "  %-16s prefix %-6s %d\n",
                    p->devs()->desc()->name()->stringNN(),
                    p->devs()->desc()->prefix(), cnt);
            }
        }
    }
    if (gd_extra_devs) {
        int cnt = 0;
        for (sEinstList *s = gd_extra_devs; s; s = s->next(), cnt++) ;
        fprintf(fp, "Extra devices: %d\n", cnt);
    }

    if (gd_vcontacts) {
        int cnt = 0;
        for (sVContact *v = gd_vcontacts; v; v = v->next, cnt++) ;
        fprintf(fp, "Virtual contacts: %d\n", cnt);
    }

    if (gd_master_list) {
        fprintf(fp, "Master list:\n");
        for (CDm *m = gd_master_list; m; m = (CDm*)m->tab_next()) {
            int cnt = 0;
            CDc_gen cgen(m);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
                cnt++;
            fprintf(fp, "  %-16s %d\n", m->celldesc()->cellname()->string(),
                cnt);
        }
    }

    if (gd_flatten_tab) {
        int cnt = gd_flatten_tab->allocated();
        fprintf(fp, "Flattened instances in table: %d\n", cnt);
    }

    if (gd_groups) {
        int vcnt = 0;
        int grp = -1;
        int maxo = 0;
        for (int i = 0; i < gd_asize; i++) {
            if (ExtErrLog.verbose()) {
                fprintf(fp, "Group %d:\n", i);
                gd_groups[i].dump(fp);
            }
            sGroup &g = gd_groups[i];
            if (!g.net() || !g.net()->objlist())
                vcnt++;
            else {
                int ocnt = 0;
                for (CDol *o = g.net()->objlist(); o; o = o->next, ocnt++) ;
                if (ocnt > maxo) {
                    maxo = ocnt;
                    grp = i;
                }
            }
        }
        fprintf(fp, "Virtual groups: %d\n", vcnt);
        fprintf(fp, "Max. group objects: %d in group %d\n", maxo, grp);
    }
}


//
// The remaining cGroupDesc functions are private.
//

// Add instances to the ignored table.  Presently, these are instances
// that have some coverage of the GlobalExclude layer expression.
//
void
cGroupDesc::find_ignored()
{
    // Warning: must call globex.clear before exit.
    sLspec globex(*EX()->globalExclude());
    if (globex.tree() || globex.ldesc()) {
        CDm_gen mgen(gd_celldesc, GEN_MASTERS);
        for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
            CDs *msdesc = m->celldesc();
            if (!msdesc)
                continue;
            if (msdesc->isConnector())
                continue;
            cGroupDesc *gd = msdesc->groups();
            if (!gd)
                continue;

            CDc_gen cgen(m);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                if (!c->is_normal())
                    continue;
                SIlexprCx cx(gd_celldesc, CDMAXCALLDEPTH, &c->oBB());
                CovType ct;
                if (globex.testZlistCovNone(&cx, &ct,
                        2*Tech()->AngleSupport()) == XIok && ct != CovNone) {
                    if (!gd_ignore_tab)
                        gd_ignore_tab = new SymTab(false, false);
                    gd_ignore_tab->add((unsigned long)c, 0, false);
                }
            }
        }
    }
    globex.clear();
}


// Process objects on layers with an exclude directive.  Objects which
// have been clipped have the group set to -1, and the clipped regions
// are added to the grouping phonies.
//
// We also exclude objects that intersect the GlobalExclude layer
// expression.
//
bool
cGroupDesc::process_exclude()
{
    // For library gates, forget about exclusion since no devices will
    // be extracted.
    if (EX()->skipExtract(gd_celldesc))
        return (true);

    // Warning: must call globex.clear before exit.
    sLspec globex(*EX()->globalExclude());
    bool use_globex = (globex.tree() || globex.ldesc());

    CDl *ld;
    CDextLgen lgen(CDL_CONDUCTOR);
    while ((ld = lgen.next()) != 0) {

        // initialize all object group numbers
        if (use_globex) {
            CDg gd;
            gd.init_gen(gd_celldesc, ld);
            CDo *odesc;
            while ((odesc = gd.next()) != 0) {
                odesc->set_group(0);
                Zlist *zl = odesc->toZlist();
                SIlexprCx cx(gd_celldesc, CDMAXCALLDEPTH, zl);
                CovType ct;
                if (globex.testZlistCovNone(&cx, &ct,
                        2*Tech()->AngleSupport()) == XIok && ct != CovNone)
                    odesc->set_group(-1);
                zl->free();
            }
        }
        else {
            CDg gd;
            gd.init_gen(gd_celldesc, ld);
            CDo *odesc;
            while ((odesc = gd.next()) != 0)
                odesc->set_group(0);
        }

        if (ld->isGroundPlane())
            continue;

        if (tech_prm(ld)->exclude()) {
            sGrpGen gdesc;
            gdesc.init_gen(this, ld, &CDinfiniteBB);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->group() < 0)
                    continue;

                SIlexprCx cx(gd_celldesc, 0, &odesc->oBB());
                Zlist *zret = 0;
                tech_prm(ld)->exclude()->getZlist(&cx, &zret);
                if (zret) {
                    // zret is list of areas to exclude
                    Zlist *zx = odesc->toZlist();
                    if (!zx) {
                        zret->free();
                        continue;
                    }
                    if (!Zlist::intersect(zx, zret, false)) {
                        zret->free();
                        zx->free();
                        continue;
                    }
                    for (Zlist *z = zret; z; z = z->next)
                        Zlist::zl_andnot(&zx, &z->Z);
                    zret->free();

                    // Mark as not included.
                    odesc->set_group(-1);

                    CDo *od = zx->to_obj_list(ld);
                    if (!od)
                        continue;

                    // Save od list in group phonies.
                    if (!gd_g_phonycell)
                        gd_g_phonycell = new CDs(0, Physical);
                    while (od) {
                        CDo *ox = od;
                        od = od->next_odesc();
                        ox->set_next_odesc(0);
                        ox->set_copy(false);
                        ox->set_flag(CDoMarkExtG);
                        if (!gd_g_phonycell->insert(ox)) {
                            delete ox;
                            Errs()->add_error(
                                "Object insertion failed in group db.");
                            while (od) {
                                ox = od;
                                od = od->next_odesc();
                                delete ox;
                            }
                            globex.clear();
                            return (false);
                        }
                    }
                }
            }
        }
    }
    globex.clear();

    // Remove any gd_phonies with negative group numbers.
    if (gd_g_phonycell) {
        CDol *ol0 = 0;
        CDlgen gen(Physical);
        while ((ld = gen.next()) != 0) {
            CDg gdesc;
            gdesc.init_gen(gd_g_phonycell, ld);
            CDo *od;
            while ((od = gdesc.next()) != 0) {
                if (od->group() < 0)
                    ol0 = new CDol(od, ol0);
            }
        }
        while (ol0) {
            CDol *o = ol0;
            ol0 = ol0->next;
            gd_g_phonycell->unlink(o->odesc, false);
            delete o;
        }
    }
    return (true);
}


// Memory management for the groups field.
//
void
cGroupDesc::alloc_groups(int size)
{
    if (!gd_groups) {
        gd_groups = new sGroup[size];
        gd_asize = size;
    }
    else {
        if (size == gd_asize)
            return;
        sGroup *newgrp = new sGroup[size];
        int sz = mmMin(size, gd_asize);
        for (int i = 0; i < sz; i++) {
            newgrp[i] = gd_groups[i];
            gd_groups[i].clear();
        }
        delete [] gd_groups;
        gd_groups = newgrp;
        gd_asize = size;
    }
}


// Perform the initial grouping operations.
//
XIrt
cGroupDesc::group_objects()
{
    /***
    // This will add a tiny bit of metal under fixed terminals so that
    // no fixed terminal will be virtual.  This is not necessary, but
    // might be helpful for debugging.
    //
    CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
    if (esdesc) {
        CDp_snode *ps = (CDp_snode*)esdesc->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            CDterm *term = ps->terminal();
            if (!term || !term->is_fixed())
                continue;
            CDl *ld = term->layer();
            if (!ld)
                continue;
            BBox BB(term->lx(), term->ly(), term->lx(), term->ly());
            BB.bloat(1);
            CDo *od = new CDo(ld, &BB);
            if (!gd_g_phonycell)
                gd_g_phonycell = new CDs(0, Physical);
            gd_g_phonycell->insert(od);
        }
    }
    ***/

    ext_group::Ufb ufb;
    if (EX()->skipExtract(gd_celldesc)) {
        // For opaque cells, we ignore the internal structure, except
        // for the implementing objects of the terminals, which are
        // added below.
        int cnt = 1;
        CDs *esdesc = CDcdb()->findCell(gd_celldesc->cellname(), Electrical);
        if (esdesc) {
            CDp_snode *ps = (CDp_snode*)esdesc->prpty(P_NODE);
            for ( ; ps; ps = ps->next()) {
                if (ps->cell_terminal() && ps->cell_terminal()->get_ref())
                    cnt++;
            }
            alloc_groups(cnt);
            cnt = 1; 
            ps = (CDp_snode*)esdesc->prpty(P_NODE);
            for ( ; ps; ps = ps->next()) {
                CDsterm *term = ps->cell_terminal();
                if (!term)
                    continue;
                CDo *od = term->get_ref();
                if (od) {
                    gd_groups[cnt].add_object(od);
                    od->set_group(cnt);
                    cnt++;
                }
            }
        }
        return (XIok);
    }

    if (ExtErrLog.log_grouping() && ExtErrLog.log_fp()) {
        FILE *fp = ExtErrLog.log_fp();
        fprintf(fp,
            "\n=======================================================\n");
        fprintf(fp, "Grouping cell %s\n", gd_celldesc->cellname()->string());
    }

    // Group objects on layers top-down.
    int last = 1;  // group 0 reserved for ground plane
    CDl *ld;
    CDextLgen lgen(CDL_CONDUCTOR, CDextLgen::TopToBot);
    while ((ld = lgen.next()) != 0) {
        if (ld->isGroundPlane() && ld->isDarkField())
            // these are never grouped
            continue;

        ufb.save("Grouping objects on %s...", ld->name());
        ufb.print();

        sGrpGen gdesc;
        gdesc.init_gen(this, ld, &CDinfiniteBB);
        CDo *odesc;
        CDol *o0 = 0;
        int cnt = 0;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->type() == CDLABEL)
                continue;
            if (!odesc->is_normal())
                continue;
            if (odesc->group() < 0)
                // objects to ignore, from process_exclude()
                continue;
            o0 = new CDol(odesc, o0);
            cnt++;
        }
        if (!cnt)
            continue;

        if (Tech()->IsGroundPlaneGlobal() && ld->isGroundPlane()) {
            // In thia case, every object on the layer is assigned to
            // group 0.

            sGroupObjs *go = new sGroupObjs(o0, 0);
            alloc_groups(last);
            gd_groups[0].set_net(go);
            gd_groups[0].newnum(0);
            continue;
        }

        sGroupObjs::sort_list(o0);

        CDol **ary = new CDol*[cnt];
        cnt = 0;
        while (o0) {
            ary[cnt] = o0;
            o0 = o0->next;
            ary[cnt]->next = 0;
            CDol *oe = ary[cnt];
            for (CDol *oc = oe; oc; oc = oc->next) {
                CDol *op = 0, *on;
                for (CDol *o = o0; o; o = on) {
                    if (o->odesc->oBB().top < oc->odesc->oBB().bottom)
                        break;
                    on = o->next;
                    if (oc->odesc->intersect(o->odesc, true)) {
                        if (!op)
                            o0 = on;
                        else
                            op->next = on;
                        o->next = 0;
                        oe->next = o;
                        oe = oe->next;
                        continue;
                    }
                    op = o;
                }
            }
            cnt++;
        }
        alloc_groups(last + cnt);
        sGroup *gp = gd_groups + last;
        for (int i = 0; i < cnt; i++) {
            sGroupObjs *go = new sGroupObjs(ary[i], 0);
            gp[i].set_net(go);
            gp[i].newnum(i + last);
        }
        delete [] ary;

        if (!Tech()->IsGroundPlaneGlobal() && ld->isGroundPlane() &&
                top_level()) {

            // Find the group with the largest area, and move this to
            // group 0.  Do this in the top-level cell only.  Note: 
            // cellnames are all in name table.

            double area = 0.0;
            int mx = 0;
            for (int i = 0; i < cnt; i++) {
                double a = gp[i].net()->area();
                if (a > area) {
                    area = a;
                    mx = i;
                }
            }
            if (area > 0.0) {
                gd_groups[0] = gp[mx];
                gd_groups[0].newnum(0);
                gp[mx].clear();
                cnt--;
                for (int i = mx; i < cnt; i++) {
                    gp[i] = gp[i + 1];
                    gp[i].newnum(i + last);
                }
                gp[cnt].clear();
            }
        }
        last += cnt;
    }

    XIrt ret = combine();
    if (ret != XIok)
        return (ret);
    renumber_groups();
    // trim size;
    alloc_groups(nextindex());

    if (ExtErrLog.log_grouping() && ExtErrLog.log_fp())
        dump(ExtErrLog.log_fp());

    return (XIok);
}


// Combine groups that are connected through a Contact layer or
// through a via.
//
XIrt
cGroupDesc::combine()
{
    ext_group::Ufb ufb;
    // First, combine groups connected through Contact layers.
    ufb.save("Looking for straps...");
    ufb.print();
    CDl *ld;
    CDextLgen lgen(CDL_IN_CONTACT, CDextLgen::TopToBot);
    while ((ld = lgen.next()) != 0) {

        if (ufb.checkPrint())
            return (XIintr);
        for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {
            CDl *ld1 = via->layer1();
            if (!ld1)
                continue;
            if (EX()->groundPlaneLayerInv() && ld1 == EX()->groundPlaneLayer())
                ld1 = EX()->groundPlaneLayerInv();
            if (!ld1->isConductor())
                continue;
            sGrpGen gdesc;
            gdesc.init_gen(this, ld, &CDinfiniteBB);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (ufb.checkPrint())
                    return (XIintr);
                int grp = odesc->group();
                sGrpGen gdesc1;
                gdesc1.init_gen(this, ld1, &odesc->oBB());
                CDo *odesc1;
                while ((odesc1 = gdesc1.next()) != 0) {
                    if (ufb.checkPrint())
                        return (XIintr);
                    int g1 = odesc1->group();
                    if (g1 == grp)
                        continue;
                    if (!odesc->intersect(odesc1, false))
                        continue;

                    bool istrue = !via->tree();
                    if (!istrue) {
                        sLspec lsp;
                        lsp.set_tree(via->tree());
                        XIrt ret = lsp.testContact(gd_celldesc, CDMAXCALLDEPTH,
                            odesc, &istrue);
                        lsp.set_tree(0);
                        if (ret != XIok)
                            return (ret);
                    }
                    if (istrue)
                        reduce(grp, g1);
                }
            }
        }
    }

    // Next, combine groups connected through vias between layers.
    ufb.save("Looking for vias...");
    ufb.reset();
    ufb.print();
    lgen = CDextLgen(CDL_VIA, CDextLgen::TopToBot);
    while ((ld = lgen.next()) != 0) {
        if (ufb.checkPrint())
            return (XIintr);
        for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {
            CDl *ld1 = via->layer1();
            CDl *ld2 = via->layer2();
            if (!ld1 || !ld2 || (ld1 == ld2))
                continue;
            if (EX()->groundPlaneLayerInv()) {
                if (ld1 == EX()->groundPlaneLayer())
                    ld1 = EX()->groundPlaneLayerInv();
                else if (ld2 == EX()->groundPlaneLayer())
                    ld2 = EX()->groundPlaneLayerInv();
            }
            if (!ld1->isConductor() || !ld2->isConductor())
                continue;

            bool null_ok1 = ld1->isGroundPlane() && ld1->isDarkField();
            bool null_ok2 = ld2->isGroundPlane() && ld2->isDarkField();

            sPF gen(gd_celldesc, &CDinfiniteBB, ld, EX()->viaSearchDepth());
            CDo *odesc;
            while ((odesc = gen.next(false, false)) != 0) {
                if (ufb.checkPrint()) {
                    delete odesc;
                    return (XIintr);
                }

                sGrpGen gdesc1;
                gdesc1.init_gen(this, ld1, &odesc->oBB());
                CDo *odesc1;
                while ((odesc1 = gdesc1.next()) != 0) {
                    if (!odesc->intersect(odesc1, false))
                        continue;
                    break;
                }
                if (!odesc1 && !null_ok1) {
                    delete odesc;
                    continue;
                }

                sGrpGen gdesc2;
                gdesc2.init_gen(this, ld2, &odesc->oBB());
                CDo *odesc2;
                while ((odesc2 = gdesc2.next()) != 0) {
                    if (!odesc->intersect(odesc2, false))
                        continue;
                    break;
                }
                if (!odesc2 && !null_ok2) {
                    delete odesc;
                    continue;
                }

                int g1 = odesc1->group();
                int g2 = odesc2->group();
                if (g1 == g2) {
                    delete odesc;
                    continue;
                }

                bool istrue;
                XIrt ret = cExt::isConnection(gd_celldesc, via, odesc,
                    odesc1, odesc2, &istrue);
                delete odesc;
                if (ret != XIok)
                    return (ret);
                if (istrue)
                    reduce(g1, g2);
            }
        }
    }
    return (XIok);
}


// Compact the group numbering, replace larger with smaller.
//
void
cGroupDesc::reduce(int n1, int n2)
{
    if (n1 == n2)
        return;
    int newn = n1 < n2 ? n1 : n2;
    int oldn = n1 > n2 ? n1 : n2;

    if (oldn >= gd_asize || newn >= gd_asize) {
        ExtErrLog.add_err(
            "In %s, reduce: group index out of range, %d %d %d.",
            gd_celldesc->cellname()->string(), oldn, newn, gd_asize);
        return;
    }

    sGroup &ngrp = gd_groups[newn];
    sGroup &ogrp = gd_groups[oldn];
    ogrp.newnum(newn);

    // update the net
    if (!ngrp.net())
        ngrp.set_net(ogrp.net());
    else if (ogrp.net()) {
        ngrp.net()->BB().add(&ogrp.net()->BB());
        CDol *ol = ngrp.net()->objlist();
        while (ol->next)
            ol = ol->next;
        ol->next = ogrp.net()->objlist();
        ogrp.net()->set_objlist(0);
        delete ogrp.net();
    }
    ogrp.set_net(0);

    // teminals
    CDpin *tl = ogrp.termlist();
    for ( ; tl; tl = tl->next()) {
        if (!tl->term()->get_ref() && tl->term()->get_v_group() >= 0)
            tl->term()->set_v_group(newn);
    }
    tl = ngrp.termlist();
    if (!tl)
        ngrp.set_termlist(ogrp.termlist());
    else {
        while (tl->next())
            tl = tl->next();
        tl->set_next(ogrp.termlist());
    }

    // device contacts
    sDevContactList *cl = ngrp.device_contacts();
    if (!cl)
        ngrp.set_device_contacts(ogrp.device_contacts());
    else {
        while (cl->next())
            cl = cl->next();
        cl->set_next(ogrp.device_contacts());
    }

    // subckt contacts
    sSubcContactList *sl = ngrp.subc_contacts();
    if (!sl)
        ngrp.set_subc_contacts(ogrp.subc_contacts());
    else {
        while (sl->next())
            sl = sl->next();
        sl->set_next(ogrp.subc_contacts());
    }

    // netname
    if (!ngrp.netname())
        ngrp.set_netname(ogrp.netname(), ogrp.netname_origin());
    else if (ogrp.netname() && ngrp.netname() != ogrp.netname()) {
        if (ogrp.netname_origin() > ngrp.netname_origin())
            ngrp.set_netname(ogrp.netname(), ogrp.netname_origin());
    }

    // clear unused group
    gd_groups[oldn].clear();
}


// Modify groups and the entries to compact the group indexing.  If
// map_ret is not null, a map will be created and returned, with
// newnum = map[oldnum].
//
void
cGroupDesc::renumber_groups(int **map_ret)
{
    int *map = 0;
    if (map_ret) {
        map = new int[gd_asize];
        map[0] = 0;
        *map_ret = map;
        int foo = 0;
        for (int i = 1; i < gd_asize; i++) {
            if (!has_net_or_terms(i))
                foo++;
            map[i] = i - foo;
        }
    }
    for (int i = 1; i < gd_asize; i++) {
        if (!has_net_or_terms(i)) {
            int j;
            for (j = i+1; j < gd_asize; j++) {
                if (has_net_or_terms(j))
                    break;
            }
            if (j == gd_asize)
                return;
            gd_groups[j].newnum(i);
            gd_groups[i] = gd_groups[j];
            gd_groups[j].clear();
        }
    }
}
// End of cGroupDesc functions.


// Add an object to the net.
//
void
sGroup::add_object(CDo *odesc)
{
    if (g_net) {
        g_net->BB().add(&odesc->oBB());
        g_net->set_objlist(new CDol(odesc, g_net->objlist()));
    }
    else
        g_net = new sGroupObjs(new CDol(odesc, 0), 0);
}


// Recursively test if a net connects to wire only, no devices. 
// Return true if so.
//
bool
sGroup::is_wire_only()
{
    if (g_device_contacts)
        return (false);
    for (sSubcContactList *cl = g_subc_contacts; cl; cl = cl->next()) {
        if (!cl->contact()->is_wire_only())
            return (false);
    }
    return (true);
}


// Assign a new number to the group.
//
void
sGroup::newnum(int num)
{
    // nets
    if (g_net) {
        for (CDol *ol = g_net->objlist(); ol; ol = ol->next)
            ol->odesc->set_group(num);
    }

    // teminals
    for (CDpin *tl = g_termlist; tl; tl = tl->next())
        if (!tl->term()->get_ref() && tl->term()->get_v_group() >= 0)
            tl->term()->set_v_group(num);

    // device contacts
    for (sDevContactList *cl = g_device_contacts; cl; cl = cl->next())
        cl->contact()->set_group(num);

    // subckt contacts
    for (sSubcContactList *sl = g_subc_contacts; sl; sl = sl->next())
        sl->contact()->set_parent_group(num);
}


// Return the number of connections.
//
int
sGroup::numentries()
{
    int num = 0;
    for (CDpin *t = g_termlist; t; t = t->next(), num++) ;
    for (sDevContactList *c = g_device_contacts; c; c = c->next(), num++) ;
    for (sSubcContactList *s = g_subc_contacts; s; s = s->next(), num++) ;
    return (num);
}


// Remove objects with the CDoMarkExtE flag set.  These are in the
// extraction phony cell, which is being cleared.
//
void
sGroup::purge_e_phonies()
{
    if (!g_net)
        return;
    CDol *op = 0, *on;
    for (CDol *o = g_net->objlist(); o; o = on) {
        on = o->next;
        if (o->odesc->has_flag(CDoMarkExtE)) {
            if (op)
                op->next = on;
            else {
                g_net->set_objlist(on);
                if (!on) {
                    delete g_net;
                    g_net = 0;
                }
            }
            delete o;
            continue;
        }
        op = o;
    }
}


namespace {
    // Where to place the group number label.
    //
    void
    find_loc(CDo *odesc, int *x, int *y)
    {
        switch (odesc->type()) {
        default:
            *x = odesc->oBB().left;
            *y = odesc->oBB().bottom;
            break;
        case CDWIRE:
            *x = OWIRE(odesc)->points()[0].x;
            *y = OWIRE(odesc)->points()[0].y;
            break;
        case CDPOLYGON:
            *x = OPOLY(odesc)->points()[0].x;
            *y = OPOLY(odesc)->points()[0].y;
            break;
        }
    }
}


// Display the group number or node name.
//
void
sGroup::show(WindowDesc *wdesc, BBox *eBB)
{
    if (g_net && g_net->objlist()) {
        int delta = wdesc->LogScale(DSP_DEF_PTRM_TXTHT + DSP_DEF_PTRM_TXTHT/2);
        char buf[128];
        if (EX()->isShowingNodes()) {
            const char *nn =
                SCD()->nodeName(CurCell(Electrical, true), g_node);
            strcpy(buf, nn);
        }
        else
            mmItoA(buf, g_net->objlist()->odesc->group());

        for (CDol *l = g_net->objlist(); l; l = l->next) {
            if (!l->odesc->is_normal())
                continue;
            int x, y, w, h;
            DSP()->DefaultLabelSize(buf, wdesc->Mode(), &w, &h);
            w = (w*delta)/h;
            h = delta;
            find_loc(l->odesc, &x, &y);
            if (eBB) {
                if (eBB->left > x)
                    eBB->left = x;
                if (eBB->bottom > y)
                    eBB->bottom = y;
                if (eBB->right < x + w)
                    eBB->right = x + w;
                if (eBB->top < y + h)
                    eBB->top = y + h;
            }
            else {
                if (l->odesc->ldesc()->isInvisible())
                    continue;
                wdesc->ShowLabel(buf, x, y, w, h, 0);
            }
        }
    }
}


void
sGroup::dump(FILE *fp)
{
    if (g_netname) {
        fprintf(fp, "  Name: %s", g_netname->string());
        const char *f = "associated node";
        if (g_nn_origin == NameFromLabel)
            f = "label";
        if (g_nn_origin == NameFromTerm)
            f = "terminal";
        fprintf(fp, " (from %s)\n", f);
    }
    if (g_net && g_net->objlist()) {
        fprintf(fp, "  Net objects:\n");
        CDs *sd = g_net->mk_cell(0, 0);
        if (sd) {
            CDsLgen lgen(sd);
            lgen.sort();
            CDl *ld;
            unsigned int tot = 0;
            while ((ld = lgen.next()) != 0) {
                CDtree *t = sd->db_find_layer_head(ld);
                if (!t)
                    continue;
                unsigned int n = t->num_elements();
                if (n > 0)
                    fprintf(fp, "    %s %u\n", ld->name(), n);
                tot += n;
            }
            fprintf(fp, "    total %u\n", tot);
            delete sd;
        }
    }
    else
        fprintf(fp, "  Virtual net\n");
    if (g_termlist) {
        fprintf(fp, "  Terminal");
        if (g_termlist->next())
            putc('s', fp);
        putc(':', fp);
        for (CDpin *t = g_termlist; t; t = t->next())
            fprintf(fp, " %s", t->term()->name()->string());
        putc('\n', fp);
    }
    int ndc = 0;
    for (sDevContactList *c = g_device_contacts; c; c = c->next())
        ndc++;
    int nsc = 0;
    for (sSubcContactList *c = g_subc_contacts; c; c = c->next())
        nsc++;
    fprintf(fp, "  Contacts: device %-5d subcircuit %-5d\n", ndc, nsc);
    fprintf(fp, "  Misc: node %-5d hint_node %-5d split_group %-5d flags %x\n",
        g_node, g_hint_node, g_split_group, g_flags);
}
// End of sGroup functions.

