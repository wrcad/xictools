
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
 $Id: ext_nets.cc,v 5.109 2017/03/16 05:19:33 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_nets.h"
#include "ext_errlog.h"
#include "sced.h"
#include "sced_param.h"
#include "sced_nodemap.h"
#include "cd_celldb.h"
#include "cd_netname.h"
#include "cd_hypertext.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "filestat.h"
#include "spnumber.h"
#include <algorithm>


//#define PRM_DEBUG
#define SP_DEBUG

/*========================================================================*
 *
 *  Functions for creating and maintaining connectivity lists
 *
 *========================================================================*/


// Constructor.
//
sElecNetList::sElecNetList(CDs *sdesc)
{
    if (sdesc && sdesc->owner())
        sdesc = sdesc->owner();
    et_sdesc = sdesc;
    et_list = 0;
    et_fcells = 0;
    et_ftab = 0;
    et_maxix = 0;
    et_size = 0;
    if (!sdesc || !sdesc->isElectrical())
        return;

    bool has_flat = false;

    et_sdesc->checkPhysTerminals(false);  // This creates instance terminals.
    SCD()->connect(et_sdesc);
    cNodeMap *nmap = et_sdesc->nodes();
    if (!nmap)
        return;
    et_size = nmap->countNodes();
    et_list = new sEnode[et_size];

    // Add terminals from subcircuits and devices.
    CDm_gen mgen(et_sdesc, GEN_MASTERS);
    for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
        CDs *msdesc = md->celldesc();
        if (!msdesc)
            continue;
        CDc_gen cgen(md);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (!cdesc->is_normal())
                continue;

            // Don't include NOPHYS devices.
            if (cdesc->prpty(P_NOPHYS))
                continue;

            // Don't include wire capacitors.
            CDp_name *pa = (CDp_name*)cdesc->prpty(P_NAME);
            if (pa && pa->assigned_name() &&
                    lstring::prefix(WIRECAP_PREFIX, pa->assigned_name()))
                continue;

            // Don't include the terminals of flattened
            // subcircuits, but we make space for the lists to
            // be inserted.

            bool isflat = false;
            if (should_flatten(cdesc, sdesc)) {
                has_flat = true;
                isflat = true;
            }
            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                int vix = 0;
                CDgenRange rgen(pr);
                while (rgen.next(0)) {
                    CDp_cnode *px;
                    if (!vix)
                        px = pn;
                    else {
                        px = pr->node(0, vix, pn->index());
                        if (!px)
                            continue;
                    }
                    if (px && px->enode() >= 0 && px->inst_terminal()) {
                        if (px->enode() >= (int)et_size) {
                            // This should not happen.
                            int i = et_size;
                            et_size = px->enode() + 10;
                            sEnode *newterms = new sEnode[et_size];
                            for (int j = 0; j < i; j++)
                                newterms[j] = et_list[j];
                            delete [] et_list;
                            et_list = newterms;
                        }
                        if (px->enode() > (int)et_maxix)
                            et_maxix = px->enode();
                        if (!isflat) {
                            CDcterm *term = px->inst_terminal();
                            CDcont *t = new CDcont(term, 0);
                            t->set_next(et_list[px->enode()].conts);
                            et_list[px->enode()].conts = t;
                        }
                    }
                    vix++;
                }
            }
        }
    }

    // Add the cell's connection points.
    update_pins();

    // Add the names.
    for (unsigned int i = 1; i <= et_maxix; i++)
        et_list[i].name = nmap->mapStab(i);

    if (has_flat) {
        // Now added flattened cells.
        mgen = CDm_gen(et_sdesc, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
            CDs *msdesc = md->celldesc();
            if (!msdesc || msdesc->elecCellType() != CDelecSubc)
                continue;
            CDc_gen cgen(md);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                if (!c->is_normal())
                    continue;
                // Don't include NOPHYS devices.
                if (c->prpty(P_NOPHYS))
                    continue;

                check_flatten(c);
            }
        }
        for (sdlink *sl = et_fcells; sl; sl = sl->next) {
            for (unsigned int i = 0; i < sl->width; i++) {
                CDs *sd = sl->sdcopy[i];
                if (!sd)
                    continue;
                mgen = CDm_gen(sd, GEN_MASTERS);
                for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
                    CDs *msdesc = md->celldesc();
                    if (!msdesc || msdesc->elecCellType() != CDelecSubc)
                        continue;
                    CDc_gen cgen(md);
                    for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
                        check_flatten(c);
                }
            }
        }
    }
    if (ExtErrLog.log_associating() && ExtErrLog.verbose() &&
            ExtErrLog.log_fp())
        dump(ExtErrLog.log_fp());
}


sElecNetList::~sElecNetList()
{
    if (et_list) {
        for (unsigned int i = 0; i <= et_maxix; i++) {
            CDcont::destroy(et_list[i].conts);
            CDpin::destroy(et_list[i].pins);
        }
        delete [] et_list;
    }
    et_fcells->free();
    delete et_ftab;
}


// Static function.
// Return true if the instance should be flattened into the parent. 
// If the instance is vectored, the true return indicates that all
// sections can be flattened.  If the cdesc is a copy from a flattened
// parent, cdesc->parent points to the cell copy, and not the actual
// parent, which is why this is passed separately.
//
// After first-pass association, if all physical subcircuits
// associate, and there are extra electrical subcircuits, these will
// be flattened, and the circuit reassociated.
//
bool
sElecNetList::should_flatten(const CDc *cdesc, CDs *parent)
{
    if (!cdesc || !parent)
        return (false);
    CDs *mstr = cdesc->masterCell();
    if (!mstr)
        return (false);

    // Only subcircuits can be flattened.
    if (mstr->elecCellType() != CDelecSubc)
        return (false);

    // Note the logic for master and instance flatten properties. 
    // It the master has the property, it works as "not-flatten"
    // for the instance.
    //
    bool fm = (mstr->prpty(P_FLATTEN) != 0);
    bool fi = (cdesc->prpty(P_FLATTEN) != 0);
    if ((!fm && fi) || (fm && !fi))
        return (true);
    if (fm || fi)
        return (false);

    int ninst = 0;
    CDcbin pcbin(parent);
    if (pcbin.phys()) {
        cGroupDesc *gd = pcbin.phys()->groups();
        if (gd) {
            for (sSubcList *sl = gd->subckts(); sl; sl = sl->next()) {
                CDs *ms = sl->subs()->cdesc()->masterCell();
                if (!ms)
                    continue;
                if (ms->cellname() == mstr->cellname()) {
                    for (sSubcInst *si = sl->subs(); si; si = si->next())
                        ninst++;
                }
            }
        }
        if (!ninst)
            return (true);
    }

    return (false);
}


// Static function.
// Return the total number of scalar placements of the master of cdesc
// in parent.
//
int
sElecNetList::count_sections(const CDc *cdesc, CDs *parent)
{
    if (!cdesc || !parent)
        return (0);
    CDcellName cname = cdesc->cellname();
    if (!cname)
        return (0);

    int cnt = 0;
    CDm_gen emgen(parent, GEN_MASTERS);
    for (CDm *md = emgen.m_first(); md; md = emgen.m_next()) {
        if (md->cellname() == cname) {
            CDc_gen cgen(md);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                CDp_range *pr = (CDp_range*)c->prpty(P_RANGE);
                cnt += pr ? pr->width() : 1;
            }
            break;
        }
    }
    CDs *sdp = CDcdb()->findCell(parent->cellname(), Physical);
    cGroupDesc *gd = sdp ? sdp->groups() : 0;
    if (gd && gd->elec_nets()) {
        sElecNetList *el = gd->elec_nets();
        for (sdlink *sl = el->et_fcells; sl; sl = sl->next) {
            for (unsigned int i = 0; i < sl->width; i++) {
                CDs *sd = sl->sdcopy[i];
                if (!sd)
                    continue;
                CDm_gen mgen(sd, GEN_MASTERS);
                for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
                    CDs *msdesc = md->celldesc();
                    if (!msdesc || msdesc->elecCellType() != CDelecSubc)
                        continue;
                    CDc_gen cgen(md);
                    for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                        CDp_range *pr = (CDp_range*)c->prpty(P_RANGE);
                        cnt += pr ? pr->width() : 1;
                    }
                }
            }
        }
    }
    return (cnt);
}


// Clear and add the pins.  The physical terms may have been added or
// removed since last call.
//
void
sElecNetList::update_pins()
{
    for (unsigned int i = 0; i < et_size; i++) {
        CDpin::destroy(et_list[i].pins);
        et_list[i].pins = 0;
    }

    CDp_snode *pn = (CDp_snode*)et_sdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        CDsterm *term = pn->cell_terminal();
        if (term && pn->enode() >= 0) {
            if (pn->enode() >= (int)et_size) {
                int i = et_size;
                et_size = pn->enode() + 10;
                sEnode *newterms = new sEnode[et_size];
                for (int j = 0; j < i; j++)
                    newterms[j] = et_list[j];
                delete [] et_list;
                et_list = newterms;
            }
            if (pn->enode() > (int)et_maxix)
                et_maxix = pn->enode();
            CDpin *t = new CDpin(term, 0);
            t->set_next(et_list[pn->enode()].pins);
            et_list[pn->enode()].pins = t;
        }
    }
}


// Remove the terminals associated with cdesc from the listing.
//
bool
sElecNetList::remove(const CDc *cdesc, unsigned int vec_ix)
{
    if (!cdesc || !et_sdesc)
        return (true);
    CDs *sd = cdesc->parent();
    if (!sd || sd != et_sdesc)
        return (false);

    CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
    CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {

        CDp_cnode *pn1;
        if (vec_ix && pr)
            pn1 = pr->node(0, vec_ix, pn->index());
        else
            pn1 = pn;

        if (pn1 && pn1->enode() >= 0 && pn1->inst_terminal()) {
            CDcterm *term = pn1->inst_terminal();

            CDcont *tprev = 0, *tnext;
            for (CDcont *t = et_list[pn1->enode()].conts; t; t = tnext) {
                tnext = t->next();
                if (t->term() == term) {
                    if (tprev)
                        tprev->set_next(tnext);
                    else
                        et_list[pn1->enode()].conts = tnext;
                    delete t;
                    continue;
                }
                tprev = t;
            }
        }
    }
    while (!et_list[et_maxix].conts && !et_list[et_maxix].pins && et_maxix)
        et_maxix--;
    return (true);
}


namespace {

    // Return an array containing an ordered list of the instance node
    // properties.
    //
    void get_nodes(const CDterm *term, CDp_cnode ***pary, int *psz)
    {
        *pary = 0;
        *psz = 0;
        CDc *cd = term->instance();
        if (!cd)
            return;

        CDp_cnode *pc0 = (CDp_cnode*)cd->prpty(P_NODE);
        unsigned int ix = 0;
        for (CDp_cnode *pc = pc0; pc; pc = pc->next()) {
            if (pc->index() > ix)
                ix = pc->index();
        }
        ix++;
        CDp_cnode **ary = new CDp_cnode*[ix];
        memset(ary, 0, ix*sizeof(CDp_cnode*));
        if (term->inst_index() > 0) {
            CDp_range *pr = (CDp_range*)cd->prpty(P_RANGE);
            if (!pr) {
                delete [] ary;
                return;
            }
            int vix = term->inst_index();
            for (CDp_cnode *pc = pc0; pc; pc = pc->next()) {
                ary[pc->index()] = pr->node(0, vix, pc->index());
                if (!ary[pc->index()]) {
                    delete [] ary;
                    return;
                }
            }
        }
        else {
            for (CDp_cnode *pc = pc0; pc; pc = pc->next())
                ary[pc->index()] = pc;
        }
        *psz = ix;
        *pary = ary;
    }
}


// Return true if i and j are permutable nodes in the circuit.  We do
// a simple test here, improvement is needed.
//
bool
sElecNetList::is_permutable(unsigned int i, unsigned int j)
{
    if (i > et_maxix || j > et_maxix)
        return (false);

    // Make sure that the connections are similar.  We allow
    // connections to devices only.

    int c1 = 0;
    for (CDcont *t = et_list[i].conts; t; t = t->next()) {
        CDc *cd = t->term()->instance();
        if (!cd)
            continue;
        CDs *sd = cd->masterCell();
        if (!sd || !sd->isDevice())
            return (false);
        c1++;
    }
    int c2 = 0;
    for (CDcont *t = et_list[j].conts; t; t = t->next()) {
        CDc *cd = t->term()->instance();
        if (!cd)
            continue;
        CDs *sd = cd->masterCell();
        if (!sd || !sd->isDevice())
            return (false);
        c2++;
    }
    if (c1 != c2)
        return (false);

    CDcterm **terms1 = new CDcterm*[c1];
    CDcterm **terms2 = new CDcterm*[c1];
    memset(terms2, 0, c1*sizeof(CDterm*));
    c1 = 0;
    for (CDcont *t = et_list[i].conts; t; t = t->next()) {
        CDc *cd = t->term()->instance();
        if (!cd)
            continue;
        terms1[c1++] = t->term();
    }
    for (int k = 0; k < c1; k++) {
        CDcellName dname = terms1[k]->instance()->cellname();
        CDnetName tname = terms1[k]->master_name();
        if (!tname) {
            delete [] terms1;
            delete [] terms2;
            return (false);
        }
        sDevDesc *dd = EX()->findDeviceDesc(dname->string());
        if (!dd) {
            delete [] terms1;
            delete [] terms2;
            return (false);
        }
        for (CDcont *t = et_list[j].conts; t; t = t->next()) {
            CDc *cd = t->term()->instance();
            if (!cd)
                continue;
            CDcterm *term = t->term();
            if (cd->cellname() == dname) {
                CDnetName mn = term->master_name();
                if (mn == tname ||
                        (dd->is_permute(mn) && dd->is_permute(tname))) {
                    terms2[k] = term;
                    break;
                }
            }
        }
        if (!terms2[k]) {
            delete [] terms1;
            delete [] terms2;
            return (false);
        }
    }

    // We know that each pair of terms is a similar contact to a
    // similar device.

    for (int k = 0; k < c1; k++) {
        CDc *cd1 = terms1[k]->instance();
        CDc *cd2 = terms2[k]->instance();

        // If the same device, we know that the terms are permutable
        // from above, so we're good.
        if (cd1 == cd2)
            continue;

        // Otherwise, we require that the other device contacts
        // connect to identical nodes.

        CDp_cnode *pn = terms1[k]->node_prpty();
        if (!pn) {
            delete [] terms1;
            delete [] terms2;
            return (false);
        }
        int skipix = pn->index();

        CDp_cnode **ary1;
        int sz1;
        get_nodes(terms1[k], &ary1, &sz1);
        if (!ary1) {
            delete [] terms1;
            delete [] terms2;
            return (false);
        }

        CDp_cnode **ary2;
        int sz2;
        get_nodes(terms2[k], &ary2, &sz2);
        if (!ary2) {
            delete [] terms1;
            delete [] terms2;
            delete [] ary1;
            return (false);
        }

        if (sz1 != sz2) {
            delete [] terms1;
            delete [] terms2;
            delete [] ary1;
            delete [] ary2;
            return (false);
        }
        sDevDesc *dd = EX()->findDeviceDesc(cd1->cellname()->string());
        for (int m = 0; m < sz1; m++) {
            if (m == skipix)
                continue;
            if (ary1[m]->enode() == ary2[m]->enode())
                continue;
            if (dd->is_permute(ary1[m]->inst_terminal()->master_name()) &&
                    dd->is_permute(ary2[m]->inst_terminal()->master_name()))
                continue;
            // Uh oh, no good.
            delete [] terms1;
            delete [] terms2;
            delete [] ary1;
            delete [] ary2;
            return (false);
        }
        delete [] ary1;
        delete [] ary2;
    }
    delete [] terms1;
    delete [] terms2;
    return (true);
}


void
sElecNetList::check_flatten(CDc *cdesc)
{
    if (!should_flatten(cdesc, et_sdesc))
        return;
    CDs *msdesc = cdesc->masterCell();
    if (!msdesc)
        return;
    bool pset = false;
    CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
    CDgenRange rgen(pr);
    int vec_ix = 0;
    while (rgen.next(0)) {
        if (!pset) {
            if (EX()->paramCx()) {
                CDp_user *pp = (CDp_user*)cdesc->prpty(P_PARAM);
                char *pstr =
                    pp ? hyList::string(pp->data(), HYcvPlain, true) : 0;
                EX()->paramCx()->push(msdesc, pstr);
#ifdef PRM_DEBUG
                printf("%s\n---\n", pstr);
                EX()->paramTab()->dump();
                printf("***\n");
#endif
                delete [] pstr;
            }
            pset = true;
        }
        ExtErrLog.add_log(ExtLogAssoc, "Smashing %s %d into %s schematic.",
            cdesc->cellname()->string(), vec_ix,
            et_sdesc->cellname()->string());
        if (!flatten(cdesc, vec_ix)) {
            ExtErrLog.add_err(
                "In %s schematic, flatten of instance of %s failed: %s.",
                et_sdesc->cellname()->string(),
                msdesc->cellname()->string(),
                Errs()->get_error());
        }
        vec_ix++;
    }
    if (pset) {
        if (EX()->paramCx())
            EX()->paramCx()->pop();
    }
}


namespace {
    void rename_term(CDcterm *term, const char *instname, const CDp_range *pr,
        int vec_ix)
    {
        CDp_cnode *pc = term->node_prpty();
        if (!pc)
            return;
        sLstr lstr;
        if (CD()->GetSubcCatmode() == cCD::SUBC_CATMODE_SPICE3) {
            lstr.add_c(*pc->term_name()->string());
            lstr.add_c(CD()->GetSubcCatchar());
            lstr.add(instname);
            lstr.add_c(CD()->GetSubcCatchar());
            lstr.add(pc->term_name()->string() + 1);
        }
        else {
            // cCD::SUBC_CATMODE_WR
            lstr.add(pc->term_name()->string());
            lstr.add_c(CD()->GetSubcCatchar());
            lstr.add(instname);
        }
        if (pr) {
            int n;
            if (pr->beg_range() > pr->end_range())
                n = pr->beg_range() - vec_ix;
            else
                n = pr->beg_range() + vec_ix;

            lstr.add_c(cTnameTab::subscr_open());
            lstr.add_i(n);
            lstr.add_c(cTnameTab::subscr_close());
        }
        pc->set_term_name(lstr.string());
    }


    void rename_inst(const CDc *cdesc, const char *instname,
        const CDp_range *pr, int vec_ix)
    {
        CDp_name *pna = (CDp_name*)cdesc->prpty(P_NAME);
        if (!pna || !isalpha(pna->key()))
            return;
        sLstr lstr;
        if (CD()->GetSubcCatmode() == cCD::SUBC_CATMODE_SPICE3) {
            const char *nm = cdesc->getBaseName(pna);
            lstr.add_c(*nm);
            lstr.add_c(CD()->GetSubcCatchar());
            lstr.add(instname);
            lstr.add_c(CD()->GetSubcCatchar());
            lstr.add(nm + 1);
        }
        else {
            // cCD::SUBC_CATMODE_WR
            lstr.add(cdesc->getBaseName(pna));
            lstr.add_c(CD()->GetSubcCatchar());
            lstr.add(instname);
        }
        if (pr) {
            int n;
            if (pr->beg_range() > pr->end_range())
                n = pr->beg_range() - vec_ix;
            else
                n = pr->beg_range() + vec_ix;

            lstr.add_c(cTnameTab::subscr_open());
            lstr.add_i(n);
            lstr.add_c(cTnameTab::subscr_close());
        }
        pna->set_assigned_name(lstr.string());
    }


    // Update instance parameter strings, making use of the present
    // parameter table.  This may be needed for extracted device
    // parameter comparisons in LVS.  It is also needed when device
    // multiplier or finger number is parameterized.
    //
    void update_params(CDs *sd)
    {
        if (!EX()->paramCx())
            return;

        // Parameter expression evaluation is expensive so use hashing
        // to avoid repeating the same evaluation.
        SymTab tab(true, true);

        CDm_gen mgen(sd, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
            CDs *msdesc = md->celldesc();
            if (!msdesc)
                continue;
            CDc_gen cgen(md);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                CDp_user *p = (CDp_user*)c->prpty(P_PARAM);
                if (!p)
                    continue;
                char *pstr = hyList::string(p->data(), HYcvPlain, true);
                if (!pstr)
                    continue;

                int scnt = 0;
                bool err = false;
                sLstr lstr;
                const char *s = pstr;
                while (*s) {
                    // Add inter-token white space, might be newlines,
                    // tabs, or regular space.
                    while (isspace(*s))
                        lstr.add_c(*s++);
                    if (!*s)
                        break;
                    const char *name_start = s;

                    // Advance past parameter name.
                    while (*s && !isspace(*s) && *s != '=')
                        s++;
                    if (!*s)
                        break;
                    const char *name_end = s;

                    // Check for equal sign.
                    while (isspace(*s))
                        s++;
                    if (*s != '=') {
                        // Uh-oh, no '=', error.
                        err = true;
                        break;
                    }

                    // Find start of value.
                    while (isspace(*s) || *s == '=')
                        s++;
                    if (!*s)
                        break;
                    const char *val_start = s;

                    // Look for a single or double quote char.
                    char qc = 0;
                    if (*s == '\'' || *s == '"')
                        qc = *s++;

                    // Advance past the value.  If not double quoted,
                    // extract the (assumed) expression, which has
                    // quotes stripped.

                    char *expr = 0;
                    if (!qc) {
                        // If the value is just a number, keep it and
                        // continue.

                        if (SPnum.parse(&s, false)) {
                            const char *t = name_start;
                            while (t != s) 
                                lstr.add_c(*t++);
                            continue;
                        }
                        while (*s && !isspace(*s))
                            s++;
                        expr = new char[s - val_start + 1];
                        int i = 0;
                        const char *t = val_start;
                        while (t < s)
                            expr[i++] = *t++;
                        expr[i] = 0;
                    }
                    else {
                        while (*s && *s != qc)
                            s++;
                        if (qc == '\'') {
                            expr = new char[s - val_start];
                            int i = 0;
                            const char *t = val_start + 1;
                            while (t < s)
                                expr[i++] = *t++;
                            expr[i] = 0;
                        }
                        if (*s == qc)
                            s++;
                    }
                    const char *val_end = s;

                    if (!expr) {
                        // No closing quote, error.
                        err = true;
                        break;
                    }

                    // Maybe we've seen this one before.
                    const char *val = (const char*)tab.get(expr);
                    if (!val) {
                        delete [] expr;
                        const char *t = name_start;
                        while (t != s) 
                            lstr.add_c(*t++);
                        continue;
                    }
                    else if (val != (const char*)ST_NIL) {
                        delete [] expr;
                        const char *t = name_start;
                        while (t != name_end)
                            lstr.add_c(*t++);
                        lstr.add_c('=');
                        lstr.add(val);
                        scnt++;
                        continue;
                    }

                    // Insert name=value.  If there is an expression
                    // and evaluation succeeds, use the return for
                    // value.
                    const char *t = name_start;
                    while (t != name_end)
                        lstr.add_c(*t++);
                    lstr.add_c('=');

                    char *oexpr = lstring::copy(expr);
                    EX()->paramCx()->update(&expr);

                    const char *xp = expr;
                    double *d = SCD()->evalExpr(&xp);
                    if (d) {
                        char buf[64];
                        sprintf(buf, "%.6e", *d);
                        lstr.add(buf);
                        tab.add(oexpr, lstring::copy(buf), false);
                        scnt++;
                    }
                    else {
                        tab.add(oexpr, 0, false);
                        t = val_start;
                        while (t < val_end)
                            lstr.add_c(*t++);
                    }
                    delete [] expr;
                }

#ifdef PRM_DEBUG
                printf("%s\n---\n", pstr);
                if (err)
                    printf("(ERROR) %s\n---\n", lstr.string());
                else
                    printf("%s\n---\n", lstr.string());
#endif
                if (!err && scnt) {
                    hyList::destroy(p->data());
                    p->set_data(new hyList(sd, lstr.string(), HYcvPlain));
                }
                delete [] pstr;
            }
        }
    }
}


// Flatten the cdesc structure into the parent.  It is assumed that
// cdesc is not currently in the listing, call remove first if this is
// not so.
//
bool
sElecNetList::flatten(const CDc *cdesc, unsigned int vec_ix,
    sEinstList **pdevs, sEinstList **psubs)
{
    if (!et_sdesc)
        return (true);

    if (!cdesc || flattened(cdesc, vec_ix))
        return (true);

    CDs *msd = cdesc->masterCell(true);
    if (!msd)
        return (true);
    if (msd->elecCellType() != CDelecSubc)
        return (true);

    CDp_snode **nodes;
    unsigned int nsize = msd->checkTerminals(&nodes);

    const char *instname = cdesc->getBaseName();

    bool ok = true;
    CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
    if (!pr && vec_ix > 0)
        return (false);
    unsigned int wd = pr ? pr->width() : 1;
    if (vec_ix >= wd)
        return (false);

    // Make a copy of its master cell.  Yes, we need a separate
    // copy for each array bit.

    CDs *sd = msd->cloneCell(0);
    sd->setName(msd->cellname());

    bool new_link = false;
    sdlink *link = (sdlink*)et_ftab->get((unsigned long)cdesc);
    if (link == (sdlink*)ST_NIL) {
        link = sdlink::alloc(cdesc, et_fcells, wd);
        new_link = true;
    }
    link->sdcopy[vec_ix] = sd;
    update_params(sd);

    sElecNetList *subl = new sElecNetList(sd);

    // Rename all of the instances.
    CDm_gen mgen(sd, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDc_gen cgen(mdesc);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
            rename_inst(c, instname, pr, vec_ix);
    }
    for (sdlink *sl = subl->et_fcells; sl; sl = sl->next) {
        for (unsigned int i = 0; i < sl->width; i++) {
            CDs *ssd = sl->sdcopy[i];
            if (!ssd)
                continue;
            mgen = CDm_gen(ssd, GEN_MASTERS);
            for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
                CDc_gen cgen(mdesc);
                for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
                    rename_inst(c, instname, pr, vec_ix);
            }
        }
    }
    if (pdevs || psubs)
        subl->list_devs_and_subs(pdevs, psubs);

    CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        if (pn->index() >= nsize) {
            ok = false;
            Errs()->add_error("instance node index too largs");
            continue;
        }
        CDp_snode *ps = nodes[pn->index()];
        if (!ps) {
            ok = false;
            Errs()->add_error("missing master node property");
            continue;
        }
        CDp_cnode *pn1;
        if (vec_ix == 0)
            pn1 = pn;
        else
            pn1 = pr->node(0, vec_ix, pn->index());
        if (!pn1) {
            ok = false;
            Errs()->add_error("vectored instance not set up properly");
            continue;
        }

        int pnode = pn1->enode();
        if (pnode < 0) {
            ok = false;
            Errs()->add_error("unconnected instance node");
            continue;
        }
        int cnode = ps->enode();
        if (cnode < 0) {
            ok = false;
            Errs()->add_error("unconnected master node");
            continue;
        }
        while (subl->et_list[cnode].pins) {
            CDpin *p = subl->et_list[cnode].pins;
            subl->et_list[cnode].pins = p->next();
            delete p;
        }
        while (subl->et_list[cnode].conts) {
            CDcont *t = subl->et_list[cnode].conts;
            subl->et_list[cnode].conts = t->next();
            CDp_cnode *pc = t->term()->node_prpty();
            if (!pc) {
                delete t;
                continue;
            }
            pc->set_enode(pnode);
            if (!t->term()->instance()) {
                delete t;
                continue;
            }
            rename_term(t->term(), instname, pr, vec_ix);
            t->set_next(et_list[pnode].conts);
            et_list[pnode].conts = t;
        }
        if (pnode > (int)et_maxix)
            et_maxix = pnode;  // "can't happen"
    }

    // Link any ground terminals.
    while (subl->et_list[0].pins) {
        // Shouldn't be any of these.
        CDpin *t = subl->et_list[0].pins;
        subl->et_list[0].pins = t->next();
        delete t;
    }
    while (subl->et_list[0].conts) {
        CDcont *t = subl->et_list[0].conts;
        subl->et_list[0].conts = t->next();
        CDp_cnode *pc = t->term()->node_prpty();
        if (!pc) {
            delete t;
            continue;
        }
        if (!t->term()->instance()) {
            delete t;
            continue;
        }
        t->set_next(et_list[0].conts);
        et_list[0].conts = t;
    }

    // Link global nets.
    for (unsigned int i = 1; i <= subl->et_maxix; i++) {
        if (!subl->et_list[i].conts && !subl->et_list[i].pins)
            continue;
        CDnetName nm = subl->et_list[i].name;
        if (!nm)
            continue;
        if (!SCD()->isGlobalNetName(nm->string()))
            continue;

        // If we don't find a match, we'll create one later.
        for (unsigned int j = 1; j <= et_maxix; j++) {
            if (nm == et_list[j].name) {

                while (subl->et_list[i].pins) {
                    CDpin *p = subl->et_list[i].pins;
                    subl->et_list[i].pins = p->next();
                    CDp_snode *tps = p->term()->node_prpty();
                    if (tps)
                        tps->set_enode(j);
                    delete p;
                }

                while (subl->et_list[i].conts) {
                    CDcont *t = subl->et_list[i].conts;
                    subl->et_list[i].conts = t->next();
                    CDp_cnode *pc = t->term()->node_prpty();
                    if (!pc) {
                        delete t;
                        continue;
                    }
                    pc->set_enode(j);
                    if (!t->term()->instance()) {
                        delete t;
                        continue;
                    }
                    rename_term(t->term(), instname, pr, vec_ix);
                    t->set_next(et_list[j].conts);
                    et_list[j].conts = t;
                }
                break;
            }
        }
    }

    // Link any remaining terminals, assigning new node numbers.
    unsigned int pnode = et_maxix + 1;
    for (unsigned int i = 1; i <= subl->et_maxix; i++) {
        if (!subl->et_list[i].conts && !subl->et_list[i].pins)
            continue;
        CDnetName nm = subl->et_list[i].name;

        if (pnode >= et_size) {
            unsigned int sz = et_size;
            et_size = pnode + 10;
            sEnode *newterms = new sEnode[et_size];
            for (unsigned int j = 0; j < sz; j++)
                newterms[j] = et_list[j];
            delete [] et_list;
            et_list = newterms;
        }
        while (subl->et_list[i].pins) {
            CDpin *p = subl->et_list[i].pins;
            subl->et_list[i].pins = p->next();
            CDp_snode *tps = p->term()->node_prpty();
            if (tps)
                tps->set_enode(pnode);
            delete p;
        }
        while (subl->et_list[i].conts) {
            CDcont *t = subl->et_list[i].conts;
            subl->et_list[i].conts = t->next();
            CDp_cnode *pc = t->term()->node_prpty();
            if (!pc) {
                delete t;
                continue;
            }
            pc->set_enode(pnode);
            if (!t->term()->instance()) {
                delete t;
                continue;
            }
            rename_term(t->term(), instname, pr, vec_ix);
            t->set_next(et_list[pnode].conts);
            et_list[pnode].conts = t;
        }
        if (SCD()->isGlobalNetName(nm->string())) {
            // If global, we didn't find an existing match
            // previously, so create one now.

            et_list[pnode].name = nm;
        }
        et_maxix = pnode++;
    }

    // The subl now contains no nets, but we need to promote the
    // instance table and cell copy list.
    if (subl->et_fcells) {
        sdlink *sl = link;
        while (sl->next)
            sl = sl->next;
        sl->next = subl->et_fcells;
        subl->et_fcells = 0;
    }
    if (subl->et_ftab) {
        if (!et_ftab)
            et_ftab = new SymTab(false, false);
        SymTabGen gen(subl->et_ftab, true);
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            et_ftab->add((unsigned long)ent->stTag, ent->stData, false);
            delete ent;
        }
    }
    delete subl;
    delete [] nodes;

    if (new_link) {
        et_fcells = link;
        if (!et_ftab)
            et_ftab = new SymTab(false, false);
        et_ftab->add((unsigned long)cdesc, et_fcells, false);
    }
    return (ok);
}


bool
sElecNetList::flattened(const CDc *cd, unsigned int vix)
{
    if (!cd || !et_ftab)
        return (false);

    sdlink *link = (sdlink*)et_ftab->get((unsigned long)cd);
    if (link == (sdlink*)ST_NIL)
        return (false);
    if (vix >= link->width)
        return (false);
    return (link->sdcopy[vix] != 0);
}


// List all electrical subcells and devices, taking into account
// flattening.
//
void
sElecNetList::list_devs_and_subs(sEinstList **pdevs, sEinstList **psubs)
{
    if (pdevs)
        *pdevs = 0;
    if (psubs)
        *psubs = 0;
    sEinstList *dlist = 0, *slist = 0;
    {
        CDm_gen mgen(et_sdesc, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
            CDs *msdesc = md->celldesc();
            if (!msdesc)
                continue;
            bool isdev = msdesc->isDevice();
            if ((isdev && !pdevs) || (!isdev && !psubs))
                continue;
            CDc_gen cgen(md);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                if (c->prpty(P_NOPHYS))
                    continue;
                CDp_range *pr = (CDp_range*)c->prpty(P_RANGE);
                CDgenRange rgen(pr);
                int vec_ix = 0;
                while (rgen.next(0)) {
                    if (isdev)
                        dlist = new sEinstList(c, vec_ix, dlist);
                    else if (!flattened(c, vec_ix))
                        slist = new sEinstList(c, vec_ix, slist);
                    vec_ix++;
                }
            }
        }
    }

    for  (sdlink *s = et_fcells; s; s = s->next) {
        for (unsigned int i = 0; i < s->width; i++) {
            CDs *sd = s->sdcopy[i];
            if (!sd)
                continue;
            CDm_gen mgen(sd, GEN_MASTERS);
            for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
                CDs *msdesc = md->celldesc();
                if (!msdesc)
                    continue;
                bool isdev = msdesc->isDevice();
                if ((isdev && !pdevs) || (!isdev && !psubs))
                    continue;
                CDc_gen cgen(md);
                for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                    if (c->prpty(P_NOPHYS))
                        continue;
                    CDp_range *pr = (CDp_range*)c->prpty(P_RANGE);
                    CDgenRange rgen(pr);
                    int vec_ix = 0;
                    while (rgen.next(0)) {
                        if (isdev)
                            dlist = new sEinstList(c, vec_ix, dlist);
                        else if (!flattened(c, vec_ix))
                            slist = new sEinstList(c, vec_ix, slist);
                        vec_ix++;
                    }
                }
            }
        }
    }
    if (pdevs)
        *pdevs = dlist;
    if (psubs)
        *psubs = slist;
}


// Remove the terminals listed in ttab.  These are known to be bound in
// the nodes listed in ntab.
//
void
sElecNetList::purge_terminals(SymTab *ttab, SymTab *ntab)
{
    if (!ntab || !ttab || !ttab->allocated())
        return;
    unsigned int dcnt = 0;
    SymTabGen gen(ntab);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        int node = (long)ent->stTag;
        if (node > (int)et_maxix)
            continue;
        CDcont *tp = 0, *tn;
        for (CDcont *t = et_list[node].conts; t; t = tn) {
            tn = t->next();
            CDcterm *term = t->term();
            if (term && term->instance()) {
                if (ttab->get((unsigned long)term) != ST_NIL) {
                    if (tp)
                        tp->set_next(tn);
                    else
                        et_list[node].conts = tn;
                    delete t;
                    dcnt++;
                    continue;
                }
            }
            tp = t;
        }
    }
}


// Debuggery.
//
void
sElecNetList::dump(FILE *fp)
{
    fprintf(fp, "%s max_ix=%d size=%d\n", et_sdesc->cellname()->string(),
        et_maxix, et_size);
    for (unsigned int i = 1; i <= et_maxix; i++) {
        CDnetName nm = et_list[i].name;
        fprintf(fp, "  %2d %-16s:", i, nm ? nm->string() : "");

        for (CDpin *p = et_list[i].pins; p; p = p->next())
            fprintf(fp, " %s", p->term()->name()->string());
        for (CDcont *t = et_list[i].conts; t; t = t->next())
            fprintf(fp, " %s", t->term()->name()->string());
        putc('\n', fp);
    }
}
// End of sElecNetList functions.


// Put the device label markers and terminals back to the default
// (unassigned) positions, perhaps recursively, depending on args.
//
void
cExt::reset(CDcbin *cbin, bool do_labels, bool do_terms, bool do_recurs)
{
    if (!cbin)
        return;
    if (do_recurs) {
        CDm_gen mgen(cbin->elec(), GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            CDs *msdesc = mdesc->celldesc();
            if (!msdesc || msdesc->isDevice() || msdesc->isConnector() ||
                    skipExtract(msdesc))
                continue;
            CDcbin tcbin(msdesc);
            reset(&tcbin, do_labels, do_terms, do_recurs);
        }
    }
    if (do_labels) {
        sEinstList *cl0 = find_scname_labels_in_area(cbin->elec(),
            &CDinfiniteBB);
        sEinstList *cn;
        for (sEinstList *cl = cl0; cl; cl = cn) {
            cn = cl->next();
            CDp_range *pr = (CDp_range*)cl->cdesc()->prpty(P_RANGE);
            CDp_name *pn = 0;
            if (cl->cdesc_index() > 1 && pr)
                pn = pr->name_prp(0, cl->cdesc_index());
            else
                pn = (CDp_name*)cl->cdesc()->prpty(P_NAME);
            if (pn) {
                pn->set_pos_x(0);
                pn->set_pos_y(0);
                pn->set_located(false);
            }
            delete cl;
        }
        arrangeInstLabels(cbin);
    }
    if (do_terms) {
        reset_all_terms(cbin->elec());
        arrangeTerms(cbin, false);
    }
    if (cbin->phys() && (do_labels || do_terms))
        cbin->phys()->reflectBadAssoc();
}


// Create or update the electrical part of the cell from the physical
// part.  The modeflag is passed on to ExtractFromSpice() and takes the
// same values plus MEL_NOLABELS.
//
bool
cExt::makeElec(CDs *sdesc, int depth, int modeflag)
{
    // sdesc is physical
    if (!sdesc || sdesc->isElectrical())
        return (false);
    if (!CurCell(Electrical))
        CDcdb()->insertCell(sdesc->cellname()->string(), Electrical);
    CDcbin cbin(sdesc->cellname());
    SCD()->connectAll(false);
#ifdef SP_DEBUG
    char *tmpf = lstring::copy("spicefile");
#else
    char *tmpf = filestat::make_temp("xi");
#endif
    FILE *fp = fopen(tmpf, "w+");
    if (!fp) {
        delete [] tmpf;
        return (false);
    }
    if (depth < 0)
        depth = 0;
    sExtCmdBtn *b = new sExtCmdBtn[3];
    b[0] = sExtCmdBtn(opt_atom_spice, 0, 0, 0, 0, false);
    b[1] = sExtCmdBtn(opt_atom_cap, 0, 0, 0, 0, false);
    b[2] = sExtCmdBtn(opt_atom_labels, 0, 0, 0, 0, false);
    b[0].set(true);
    sDumpOpts opts(b, 3, 0);
    opts.set_depth(depth);
    if (modeflag & EFS_CLEAR)
        opts.set_spice_print_mode(PSPM_physical);
    else
        opts.set_spice_print_mode(PSPM_mixed);
    if (modeflag & EFS_WIRECAP)
        b[1].set(true);
    if (modeflag & MEL_NOLABELS)
        b[2].set(true);
    if (dumpPhysNetlist(fp, sdesc, &opts)) {
        rewind(fp);
        dspPkgIf()->CheckForInterrupt();
        if (!DSP()->Interrupt()) {
            dspPkgIf()->SetWorking(true);
            SCD()->extractFromSpice(cbin.elec(), fp, modeflag & EFS_MASK);
            dspPkgIf()->SetWorking(false);
        }
        DSP()->SetInterrupt(DSPinterNone);
    }
    fclose(fp);
#ifdef SP_DEBUG
#else
    unlink(tmpf);
#endif
    delete [] tmpf;
    return (true);
}


// Return a list of cell terminals contained within AOI if AOI has
// area, or near AOI if AOI is a point.
//
CDpin *
cExt::pointAtPins(BBox *AOI)
{
    WindowDesc *wdesc = EV()->ButtonWin();
    if (!wdesc)
        wdesc = DSP()->MainWdesc();
    int delta = (int)(DSP_DEF_PTRM_DELTA/wdesc->Ratio());
    if (AOI->left == AOI->right) {
        AOI->left -= delta;
        AOI->right += delta;
    }
    if (AOI->bottom == AOI->top) {
        AOI->bottom -= delta;
        AOI->top += delta;
    }
    return (find_pins_in_area(CurCell(Electrical, true), AOI));
}


sEinstList *
cExt::pointAtLabels(BBox *AOI)
{
    WindowDesc *wdesc = EV()->ButtonWin();
    if (!wdesc)
        wdesc = DSP()->MainWdesc();
    int delta = (int)(DSP_DEF_PTRM_DELTA/wdesc->Ratio());
    if (AOI->left == AOI->right) {
        AOI->left -= delta;
        AOI->right += delta;
    }
    if (AOI->bottom == AOI->top) {
        AOI->bottom -= delta;
        AOI->top += delta;
    }
    return (find_scname_labels_in_area(CurCell(Electrical, true), AOI));
}


namespace {
    inline bool
    tscmp(const CDsterm *ta, const CDsterm *tb)
    {
        return (strcmp(ta->name()->string(), tb->name()->string()) < 0);
    }

    inline bool
    tccmp(const CDcterm *ta, const CDcterm *tb)
    {
        return (strcmp(ta->name()->string(), tb->name()->string()) < 0);
    }

    // Alphabetically sort the terminal list by name.  Return the
    // number of terminals in the list.
    //
    int alpha_sort(CDpin *pins)
    {
        int cnt = 0;
        for (CDpin *t = pins; t; t = t->next())
            cnt++;
        if (cnt < 2)
            return (cnt);

        CDsterm **ary = new CDsterm*[cnt];

        cnt = 0;
        for (CDpin *t = pins; t; t = t->next())
            ary[cnt++] = t->term();
        std::sort(ary, ary + cnt, tscmp);

        cnt = 0;
        for (CDpin *t = pins; t; t = t->next())
            t->set_term(ary[cnt++]);
        delete [] ary;
        return (cnt);
    }


    int alpha_sort(CDcont *conts)
    {
        int cnt = 0;
        for (CDcont *t = conts; t; t = t->next())
            cnt++;
        if (cnt < 2)
            return (cnt);

        CDcterm **ary = new CDcterm*[cnt];

        cnt = 0;
        for (CDcont *t = conts; t; t = t->next())
            ary[cnt++] = t->term();
        std::sort(ary, ary + cnt, tccmp);

        cnt = 0;
        for (CDcont *t = conts; t; t = t->next())
            t->set_term(ary[cnt++]);
        delete [] ary;
        return (cnt);
    }
}


// Find all of the uninitialized terminals or those located outside of
// the physical BB, and array them outside the lower left of the cell.
// Likewise, arrange the unassigned instance labels along the right
// side.
//
void
cExt::arrangeTerms(CDcbin *cbin, bool conts_only)
{
    if (!cbin)
        return;

    CDs *esdesc = cbin->elec();
    CDs *psdesc = cbin->phys();
    if (!esdesc || !psdesc)
        return;

    CDpin *upins = find_pins_ungrouped(esdesc);
    CDcont *uconts = conts_only ? 0 : find_conts_ungrouped(esdesc);

    const int thei = 5 * INTERNAL_UNITS(CDphysDefTextHeight);
    const int twid = 5 * INTERNAL_UNITS(CDphysDefTextHeight);

    int count = alpha_sort(upins) + alpha_sort(uconts);
    int cols = (int)(sqrt((double)count));
    int x, y;
    const BBox *sBB = psdesc->BB();
    if (sBB->left == CDinfinity)
        x = -cols*twid;
    else
        x = sBB->left - cols*twid;
    if (sBB->bottom == CDinfinity)
        y = 0;
    else
        y = sBB->bottom;
    int i = 0, j = 0;
    for (CDpin *p = upins; p; p = p->next()) {
        p->term()->set_ref(0);  // Should already be true for TE_UNINIT.
        // We know terminal is not fixed.
        if (i == cols) {
            i = 0;
            j++;
        }
        p->term()->set_loc(x + i*twid, y + j*thei);
        i++;
    }
    for (CDcont *t = uconts; t; t = t->next()) {
        t->term()->set_ref(0);  // Should already be true for TE_UNINIT.
        // We know terminal is not fixed.
        if (i == cols) {
            i = 0;
            j++;
        }
        t->term()->set_loc(x + i*twid, y + j*thei);
        i++;
    }
    CDpin::destroy(upins);
    CDcont::destroy(uconts);
}


void
cExt::arrangeInstLabels(CDcbin *cbin)
{
    if (!cbin)
        return;

    CDs *esdesc = cbin->elec();
    CDs *psdesc = cbin->phys();
    if (!esdesc || !psdesc)
        return;
    const BBox *sBB = psdesc->BB();

    sEinstList *cl = find_scname_labels_unplaced(esdesc);
    if (!cl)
        return;

    const int thei = 5 * INTERNAL_UNITS(CDphysDefTextHeight);
    const int twid = 5 * INTERNAL_UNITS(CDphysDefTextHeight);
    int x, y;
    if (sBB->right == -CDinfinity)
        x = twid/2;
    else
        x = sBB->right + twid/2;
    if (sBB->top == -CDinfinity)
        y = 0;
    else
        y = sBB->top;

    while (cl) {
        CDp_range *pr = (CDp_range*)cl->cdesc()->prpty(P_RANGE);
        CDp_name *pn = 0;
        if (cl->cdesc_index() > 0 && pr)
            pn = pr->name_prp(0, cl->cdesc_index());
        else
            pn = (CDp_name*)cl->cdesc()->prpty(P_NAME);
        if (pn) {
            pn->set_pos_x(x);
            pn->set_pos_y(y);
            pn->set_located(true);
            y -= thei/2;
        }
        sEinstList *cx = cl;
        cl = cl->next();
        delete cx;
    }
}


// Export to reset subcircuit terminal locations, used in undo/redo.
//
void
cExt::placePhysSubcTerminals(const CDc *ecdesc, int ec_vecix,
    const CDc *pcdesc, unsigned int ix, unsigned int iy)
{
    if (!ecdesc || !pcdesc)
        return;
    CDs *esdesc = ecdesc->masterCell(true);
    if (!esdesc)
        return;

    CDap ap(pcdesc);
    if (ix >= ap.nx || iy >= ap.ny)
        return;

    CDp_snode **node_ary;
    unsigned int asz = esdesc->checkTerminals(&node_ary);
    if (!asz)
        return;

    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(pcdesc);
    stk.TTransMult(ix*ap.dx, iy*ap.dy);

    CDp_range *pr = (CDp_range*)ecdesc->prpty(P_RANGE);
    CDp_cnode *cpn0 = (CDp_cnode*)ecdesc->prpty(P_NODE);
    for ( ; cpn0; cpn0 = cpn0->next()) {

        CDp_cnode *cpn;
        if (ec_vecix > 0 && pr)
            cpn = pr->node(0, ec_vecix, cpn0->index());
        else
            cpn = cpn0;
        if (!cpn)
            continue;

        CDcterm *cterm = cpn->inst_terminal();
        if (!cterm)
            continue;
        unsigned int n = cpn->index();
        if (n < asz) {
            CDp_snode *spn = node_ary[n];
            if (spn && spn->cell_terminal()) {
                CDsterm *sterm = spn->cell_terminal();
                int x = sterm->lx();
                int y = sterm->ly();
                stk.TPoint(&x, &y);
                cterm->set_loc(x, y);

                // Set the vgroup to the group number.  This allows
                // the group() method to return a correct value, since
                // the t_oset is never set for instance terminals.
                //
                int node = cpn->enode();
                cGroupDesc *gd = pcdesc->parent()->groups();
                if (gd) {
                    int grp = gd->group_of_node(node);
                    if (grp >= 0)
                        cterm->set_v_group(grp);
                }

                // Set the layer field of the instance terminal to
                // the correct layer, so on-screen mark will show this.
                //
                cterm->set_layer(sterm->layer());
                cterm->set_uninit(false);
            }
        }
    }
    stk.TPop();
    delete [] node_ary;
}


// Undo the physical object association of all non-fixed terminals.
//
void
cExt::reset_all_terms(CDs *sdesc)
{
    if (!sdesc || !sdesc->isElectrical())
        return;
    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;

    // First the instance terminals.
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;
        CDelecCellType tp = msdesc->elecCellType();
        if (tp != CDelecDev && tp != CDelecSubc)
            continue;

        // Note that this returns only the 0'th component
        // terminals for vectored instances.

        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (shouldFlatten(cdesc, sdesc))
                continue;
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                CDcterm *term = pn->inst_terminal();
                if (!term)
                    continue;
                if (!term->is_fixed())
                    term->set_ref(0);
            }
        }
    }

    // Now the cell terminals.
    CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        CDsterm *term = pn->cell_terminal();
        if (!term)
            continue;
        if (!term->is_fixed())
            term->set_ref(0);
    }
}


// Return a list of cell terminals found in AOI.
//
CDpin *
cExt::find_pins_in_area(CDs *sdesc, const BBox *AOI)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;
    CDpin *p0 = 0;
    CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        CDsterm *term = pn->cell_terminal();
        if (!term)
            continue;
        if (AOI->intersect(term->lx(), term->ly(), true))
            p0 = new CDpin(term, p0);
    }
    return (p0);
}


// Return cell terminals that are TE_UNINIT or don't have a valid
// group, and are not fixed.
//
CDpin *
cExt::find_pins_ungrouped(CDs *sdesc)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;
    CDpin *p0 = 0;
    CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        CDsterm *term = pn->cell_terminal();
        if (!term)
            continue;
        if (term->is_fixed())
            continue;
        if (term->is_uninit() || term->group() == -1)
            p0 = new CDpin(term, p0);
    }
    return (p0);
}


// Return instance contacts that are TE_UNINIT or don't have a valid
// group.
//
CDcont *
cExt::find_conts_ungrouped(CDs *sdesc)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;

    CDcont *t0 = 0;
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;
        CDelecCellType tp = msdesc->elecCellType();
        if (tp != CDelecDev && tp != CDelecSubc)
            continue;

        // Note that this returns only the 0'th component
        // terminals for vectored instances.

        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (shouldFlatten(cdesc, sdesc))
                continue;
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                CDcterm *term = pn->inst_terminal();
                if (!term)
                    continue;
                if (term->is_uninit() ||
                        (!term->instance() && term->group() == -1))
                    t0 = new CDcont(term, t0);
            }
        }
    }
    return (t0);
}


// Return a list of labeled subcircuit instances whose physical name
// label is located in AOI.
//
sEinstList *
cExt::find_scname_labels_in_area(CDs *sdesc, const BBox *AOI)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;

    sEinstList *cl0 = 0;
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;
        if (msdesc->elecCellType() != CDelecSubc)
            continue;

        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (shouldFlatten(cdesc, sdesc))
                continue;

            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
            CDgenRange rgen(pr);
            int vec_ix = 0;
            while (rgen.next(0)) {

                CDp_name *pn;
                if (vec_ix == 0) {
                    pn = (CDp_name*)cdesc->prpty(P_NAME);
                    if (!pn || !pn->is_subckt())
                        break;
                }
                else
                    pn = pr->name_prp(0, vec_ix);

                Point_c px(pn->pos_x(), pn->pos_y());
                if (AOI->intersect(&px, true))
                    cl0 = new sEinstList(cdesc, vec_ix, cl0);
                vec_ix++;
            }
        }
    }
    return (cl0);
}


// Return a list of labeled subcircuit instances whose physical name
// label in not positioned over the physical cell.
//
sEinstList *
cExt::find_scname_labels_unplaced(CDs *sdesc)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;

    sEinstList *cl0 = 0;
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDs *msdesc = mdesc->celldesc();
        if (!msdesc)
            continue;
        if (msdesc->elecCellType() != CDelecSubc)
            continue;

        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (shouldFlatten(cdesc, sdesc))
                continue;

            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
            CDgenRange rgen(pr);
            int vec_ix = 0;
            while (rgen.next(0)) {

                CDp_name *pn;
                if (vec_ix == 0) {
                    pn = (CDp_name*)cdesc->prpty(P_NAME);
                    if (!pn || !pn->is_subckt())
                        break;
                }
                else
                    pn = pr->name_prp(0, vec_ix);
                if (!pn) {
                    // Range property not set up yet, counts as a match.
                    cl0 = new sEinstList(cdesc, vec_ix, cl0);
                }
                else {
                    if (!pn->located() ||
                            !cdesc->findPhysDualOfElec(vec_ix, 0, 0))
                        cl0 = new sEinstList(cdesc, vec_ix, cl0);
                }
                vec_ix++;
            }
        }
    }
    return (cl0);
}

