
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
 $Id: ext_antenna.cc,v 5.26 2016/05/28 06:24:23 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_antenna.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "geo_ylist.h"
#include "timer.h"
#include "promptline.h"


cAntParams *cAntParams::instancePtr = 0;

cAntParams::cAntParams()
{
    if (instancePtr) {
        fprintf(stderr,
            "Singleton class cAntParams is already instantiated.\n");
        exit (1);
    }
    instancePtr = this;

    ap_gate_name = 0;
    ap_mos_names = 0;
    set_gate_name("g");
}


// Private static error exit.
//
void
cAntParams::on_null_ptr()
{
    fprintf(stderr, "Singleton class cAntParams used before instantiated.\n");
    exit(1);
}
// End of cAntParams functions.


namespace {

    // Instantiate.
    cAntParams _ap_;

    unsigned long check_time;

    inline bool checkInterrupt()
    {
        if (Timer()->check_interval(check_time)) {
            if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wdraw())
                dspPkgIf()->CheckForInterrupt();
            return (XM()->ConfirmAbort());
        }
        return (false);
    }
}


bool
ant_pathfinder::find_antennae(CDs *sdesc)
{
    pf_topcell = sdesc;
    clear();
    if (!sdesc) {
        Errs()->add_error("find_antennae: null cell pointer!");
        return (false);
    }
    set_depth(CDMAXCALLDEPTH);

    if (!AP()->gate_name()) {
        Errs()->add_error("find_antennae: null gate contact name!");
        return (false);
    }
    if (!AP()->mos_names()) {
        Errs()->add_error("find_antennae: null MOS device name list!");
        return (false);
    }

    try {
        if (!find_antennae_rc(sdesc, 0))
            return (false);
    }
    catch (int) {
        Errs()->add_error("find_antennae: interrupted!");
        return (false);
    }
    return (true);
}


// Traverse the cell hierarchy looking for MOS gates.  When a MOS gate
// is found, a unique name is associated.  If this name is not found
// in the pf_gate_tab, it has not been seen before, and we will
// extract the net connected to the gate, and other connected gates.
//
bool
ant_pathfinder::find_antennae_rc(CDs *sdesc, int depth) throw(int)
{
    if (!sdesc) {
        Errs()->add_error("find_antennae_rc: null cell pointer!");
        return (false);
    }
    cGroupDesc *gdesc = sdesc->groups();
    if (!gdesc) {
        Errs()->add_error("find_antennae_rc: null group pointer in cell %s!",
            sdesc->cellname()->string());
        return (false);
    }
    if (TFull()) {
        Errs()->add_error("find_antennae_rc: transformation stack full!");
        return (false);
    }
    if (checkInterrupt())
        throw (1);

    for (const sSubcList *sl = gdesc->subckts(); sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            pf_subc_stack[depth] = s;
            CDc *cdesc = s->cdesc();
            CDs *sd = cdesc->masterCell(true);
            TPush();
            TApplyTransform(cdesc);
            TPremultiply();
            CDap ap(cdesc);
            TTransMult(s->ix()*ap.dx, s->iy()*ap.dy);
            try {
                if (!find_antennae_rc(sd, depth+1)) {
                    TPop();
                    return (false);
                }
            }
            catch (int) {
                TPop();
                throw;
            }
            TPop();
        }
    }
    for (int i = 0; i < gdesc->num_groups(); i++) {
        const sGroup *g = gdesc->group_for(i);

        if (!g->net())
            continue;

        sSubcInst *stack_bak[CDMAXCALLDEPTH];
        for (sDevContactList *c = g->device_contacts(); c; c = c->next()) {
            if (AP()->is_mos_gate(c->contact())) {
                char *name = dev_name(c->contact(), depth);
                if (!name) {
                    // can't happen
                    continue;
                }

                if (SymTab::get(pf_gate_tab, name) == ST_NIL) {

                    // Ick, have to back up the stack.
                    for (int k = 0; k < depth; k++)
                        stack_bak[k] = pf_subc_stack[k];

                    if (!depth)
                        recurse_path(gdesc, c->contact()->group(), depth);
                    else
                        recurse_up(c->contact()->group(), depth);

                    for (int k = 0; k < depth; k++)
                        pf_subc_stack[k] = stack_bak[k];

                    process(c->contact());
                    clear();
                    pf_netcnt++;
                }
                delete [] name;
            }
        }
    }
    return (true);
}


void
ant_pathfinder::process(const sDevContactInst *cx)
{
    SymTab ltab(false, false);

    // Zoidify the path, by layer.
    SymTabGen gen(pf_tab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        for (CDo *od = (CDo*)h->stData; od; od = od->next_odesc()) {
            Zlist *zl = od->toZlist();
            SymTabEnt *hh = SymTab::get_ent(&ltab, (unsigned long)od->ldesc());
            if (!hh) {
                ltab.add((unsigned long)od->ldesc(), zl, false);
                continue;
            }
            Zlist *zx = (Zlist*)hh->stData;
            hh->stData = zl;
            while (zl->next)
                zl = zl->next;
            zl->next = zx;
        }
    }

    // Replace the Zlists with a string giving the area.
    SymTabGen lgen(&ltab);
    while ((h = lgen.next()) != 0) {
        Zlist *zl = (Zlist*)h->stData;
        zl = Zlist::repartition_ni(zl);
        double a = Zlist::area(zl);
        Zlist::destroy(zl);
        char *s = new char[32];
        sprintf(s, "%.6e", a);
        h->stData = s;
    }

    BBox cBB(*cx->cBB());
    TBB(&cBB, 0);

    int ndgt = CD()->numDigits();
    sLstr lstr;
    char buf[256];
    sprintf(buf, "net %-7d (ref %.*f,%.*f %.*f,%.*f)\n", pf_netcnt,
        ndgt, MICRONS(cBB.left), ndgt, MICRONS(cBB.bottom),
        ndgt, MICRONS(cBB.right), ndgt, MICRONS(cBB.top));
    lstr.add(buf);

    double gate_area = 0.0;
    for (gate_t *gt = pf_gates; gt; gt = gt->next) {
        int len = abl_t::length(gt->bbs);
        double a = abl_t::farea(gt->bbs);
        gate_area += a;
        sprintf(buf, "  gate=%s sect=%d area=%.6e\n", gt->pathname, len, a);
        lstr.add(buf);
    }
    sprintf(buf, "  total_gate_area=%.6e\n", gate_area);
    lstr.add(buf);

    bool show_me = (!specs() && limit() == 0.0);
    double tot_net_area = 0.0;
    lgen = SymTabGen(&ltab);
    while ((h = lgen.next()) != 0) {
        const char *lname = ((CDl*)h->stTag)->name();
        double a = atof((char*)h->stData);
        double lratio = a/gate_area;
        sprintf(buf, "  layer=%s area=%s norm_to_gate=%.6e\n", lname,
            (char*)h->stData, lratio);
        lstr.add(buf);
        tot_net_area += a;
        for (const alimit_t *lim = specs(); lim; lim = lim->next) {
            if (!show_me && !strcmp(lim->al_lname, lname)) {
                if (lim->al_max_ratio > 0 && lratio > lim->al_max_ratio)
                    show_me = true;
                break;
            }
        }
    }
    double ratio = tot_net_area/gate_area;
    sprintf(buf, "  tot_net_area=%.6e norm_to_gate=%.6e\n", tot_net_area,
        ratio);
    lstr.add(buf);
    if (!show_me && limit() > 0 && ratio > limit())
        show_me = true;
    if (show_me) {
        if (pf_outfp)
            fputs(lstr.string(), pf_outfp);
        else
            fputs(lstr.string(), stdout);
    }
}


// Static function.
// Query the user for a net number, obtain and return the reference
// BB from the antenna.log file.
//
bool
ant_pathfinder::read_file(int *pnetnum, BBox *pnetBB)
{
    if (pnetnum)
        *pnetnum = -1;
    CDs *sdesc = CDcdb()->findCell(DSP()->CurCellName(), Physical);
    if (!sdesc) {
        PL()->ShowPrompt("No current physical cell!");
        return (false);
    }

    char buf[256];
    sprintf(buf, "%s.antenna.log", sdesc->cellname()->string());

    FILE *fp = fopen(buf, "r");
    if (!fp) {
        PL()->ShowPromptV("Error: can't open file %s.", buf);
        return (false);
    }
    char *in = PL()->EditPrompt("Enter antenna net number: ", 0);
    if (!in)
        return (false);
    int nnum;
    if (sscanf(in, "%d", &nnum) != 1 && nnum >= 0) {
        PL()->ShowPrompt("Bad input, expecting non-neagtive integer.");
        return (false);
    }
    if (pnetnum)
        *pnetnum = nnum;

    char *s;
    while ((s = fgets(buf, 156, fp)) != 0) {
        if (lstring::prefix("net ", s)) {
            s += 4;
            int n = atoi(s);
            if (n < nnum) 
                continue;
            if (n > nnum)
                break;
            lstring::advtok(&s);
            lstring::advtok(&s);
            double l, b, r, t;
            if (sscanf(s, "%lf,%lf %lf,%lf", &l, &b, &r, &t) != 4) {
                fclose(fp);
                PL()->ShowPromptV("Parse error in data file, bad net %d.",
                    nnum);
                return (false);
            }
            if (pnetBB) {
                pnetBB->left = INTERNAL_UNITS(l);
                pnetBB->bottom = INTERNAL_UNITS(b);
                pnetBB->right = INTERNAL_UNITS(r);
                pnetBB->top = INTERNAL_UNITS(t);
            }
            fclose(fp);
            return (true);
        }
    }
    fclose(fp);
    PL()->ShowPromptV("No such net %d.", nnum);
    return (false);
}


// Compose unique gate name.
//
char *
ant_pathfinder::dev_name(const sDevContactInst *ci, int depth)
{
    if (!ci || !ci->dev() || !ci->dev()->desc())
        return (0);
    sLstr lstr;
    for (int k = 0; k < depth; k++) {
        sSubcInst *s = pf_subc_stack[k];
        char *iname = s->instance_name();
        lstr.add(iname);
        /* I don't think this is necessary, the index is unique.
        CDap ap(s->cdesc());
        if (ap.nx > 1 || ap.ny > 1) {
            lstr.add_c('(');
            lstr.add_i(s->ix());
            lstr.add_c(',');
            lstr.add_i(s->iy());
            lstr.add_c(')');
        }
        */
        lstr.add_c(':');
    }
    lstr.add(ci->dev()->desc()->prefix());
    lstr.add_i(ci->dev()->index());
    return (lstr.string_trim());
}


abl_t *
ant_pathfinder::dev_contacts(const sDevContactInst *c)
{
    abl_t *b0 = 0;
    const sDevInst *di = c->dev();
    if (!di)
        return (0);
    if (di->mstatus() == MS_PARALLEL) {
        for (di = di->multi_devs(); di; di = di->next()) {
            for (const sDevContactInst *cc = di->contacts(); cc;
                    cc = cc->next()) {
                if (cc->cont_name() == AP()->gate_name()) {
                    b0 = new abl_t(cc->cBB(), cc->fillfct(), b0);
                    TBB(&b0->BB, 0);
                    break;
                }
            }
        }
    }
    else {
        b0 = new abl_t(c->cBB(), c->fillfct(), b0);
        TBB(&b0->BB, 0);
    }
    return (b0);
}


// Move up in the hierarchy as far as possible, along the same conductor.
// Then add the connected groups recursively through recurse_path_wg().
//
void
ant_pathfinder::recurse_up(int grp, int depth)
{
    if (checkInterrupt())
        throw (1);
    if (depth > 0 && grp >= 0) {

        sSubcInst *s = pf_subc_stack[depth - 1];
        CDc *cdesc = s->cdesc();

        CDs *parent = cdesc->parent();

        for (sSubcContactInst *c = s->contacts(); c; c = c->next()) {
            if (c->subc_group() == grp) {
                CDtf tf;
                TCurrent(&tf);
                TPop();
                try {
                    if (depth == 1)
                        recurse_path(parent->groups(), c->parent_group(), 0);
                    else
                        recurse_up(c->parent_group(), depth - 1);
                }
                catch (int) {
                    TPush();
                    TLoadCurrent(&tf);
                    throw;
                }
                TPush();
                TLoadCurrent(&tf);
                return;
            }
        }

        CDs *tsd = CDcdb()->findCell(cdesc->cellname(), Physical);
        if (tsd) {
            cGroupDesc *gd = tsd->groups();
            if (gd)
                recurse_path(gd, grp, depth);
        }
    }
}


// Descend into the hierarchy, adding the connected groups.  Groups
// are added here if they are not the ground group.  If the ground node
// is seen, set the pf_found_ground flag.
//
void
ant_pathfinder::recurse_path(cGroupDesc *gdesc, int grp, int depth)
{
    if (!gdesc || grp < 0 || grp >= gdesc->num_groups())
        return;
    if (TFull())
        return;
    if (checkInterrupt())
        throw (1);
    if (grp == 0) {
        for (const sSubcList *sl = gdesc->subckts(); sl; sl = sl->next()) {
            for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                CDc *cdesc = s->cdesc();
                CDs *sd = cdesc->masterCell(true);
                cGroupDesc *gd = sd->groups();
                if (!gd)
                    continue;
                TPush();
                TApplyTransform(cdesc);
                TPremultiply();
                CDap ap(cdesc);
            pf_subc_stack[depth] = s;
                TTransMult(s->ix()*ap.dx, s->iy()*ap.dy);
                try {
                    recurse_path(gd, 0, depth + 1);
                }
                catch (int) {
                    TPop();
                    throw;
                }
                TPop();
            }
        }
    }
    if (depth < pf_depth) {
        for (const sSubcList *sl = gdesc->subckts(); sl; sl = sl->next()) {
            for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                pf_subc_stack[depth] = s;
                for (sSubcContactInst *c = s->contacts(); c; c = c->next()) {
                    if (c->parent_group() == grp) {
                        CDc *cdesc = s->cdesc();
                        CDs *sd = cdesc->masterCell(true);
                        cGroupDesc *gd = sd->groups();
                        if (!gd)
                            continue;
                        TPush();
                        TApplyTransform(cdesc);
                        TPremultiply();
                        CDap ap(cdesc);
                        TTransMult(s->ix()*ap.dx, s->iy()*ap.dy);
                        try {
                            recurse_path(gd, c->subc_group(), depth + 1);
                        }
                        catch (int) {
                            TPop();
                            throw;
                        }
                        TPop();
                    }
                }
            }
        }
    }
    if (grp == 0)
        pf_found_ground = true;
    else {
        // Save net.
        const sGroup *g = gdesc->group_for(grp);
        sGroupObjs *gobj = g ? g->net() : 0;
        if (gobj) {
            for (CDol *o = gobj->objlist(); o; o = o->next) {
                CDo *ocopy = o->odesc->copyObjectWithXform(this);
                // Force "real object" pointer to be the copied object.
                ocopy->set_next_odesc(o->odesc);
                if (insert(ocopy) == PFerror)
                    return;
            }
        }

        // Save connected gates.
        for (sDevContactList *c = g->device_contacts(); c; c = c->next()) {
            if (AP()->is_mos_gate(c->contact())) {
                char *name = dev_name(c->contact(), depth);
                abl_t *bl = dev_contacts(c->contact());
                if (!name || !bl) {
                    // can't happen
                    continue;
                }

                // Save gates in this path.
                pf_gates = new gate_t(name, bl, pf_gates);

                // Add gate to the "visited" table if not already there.
                if (!pf_gate_tab)
                    pf_gate_tab = new SymTab(true, false);
                if (SymTab::get(pf_gate_tab, name) == ST_NIL)
                    pf_gate_tab->add(name, 0, false);
            }
        }
    }
}


// Descend, adding the ground groups.
//
void
ant_pathfinder::recurse_gnd_path(cGroupDesc *gdesc, int depth)
{
    if (!gdesc)
        return;
    if (TFull())
        return;
    for (const sSubcList *sl = gdesc->subckts(); sl; sl = sl->next()) {
        for (sSubcInst *s = sl->subs(); s; s = s->next()) {
            if (checkInterrupt())
                throw (1);
            CDc *cdesc = s->cdesc();
            CDs *sd = cdesc->masterCell(true);
            cGroupDesc *gd = sd->groups();
            if (!gd)
                continue;
            TPush();
            TApplyTransform(cdesc);
            TPremultiply();
            CDap ap(cdesc);
            TTransMult(s->ix()*ap.dx, s->iy()*ap.dy);
            try {
                recurse_gnd_path(gd, depth + 1);
            }
            catch (int) {
                TPop();
                throw;
            }
            TPop();
        }
    }

    // Save net.
    const sGroup *g = gdesc->group_for(0);
    sGroupObjs *gobj = g ? g->net() : 0;
    if (gobj) {
        for (CDol *o = gobj->objlist(); o; o = o->next) {
            CDo *ocopy = o->odesc->copyObjectWithXform(this);
            // Force "real object" pointer to be the copied object.
            ocopy->set_next_odesc(o->odesc);
            if (insert(ocopy) == PFerror)
                return;
        }
    }

    // Save connected gates.
    for (sDevContactList *c = g->device_contacts(); c; c = c->next()) {
        if (AP()->is_mos_gate(c->contact())) {
            char *name = dev_name(c->contact(), depth);
            abl_t *bl = dev_contacts(c->contact());
            if (!name || !bl) {
                // can't happen
                continue;
            }

            // Save gates in this path.
            pf_gates = new gate_t(name, bl, pf_gates);

            // Add gate to the "visited" table if not already there.
            if (!pf_gate_tab)
                pf_gate_tab = new SymTab(true, false);
            if (SymTab::get(pf_gate_tab, name) == ST_NIL)
                pf_gate_tab->add(name, 0, false);
        }
    }
}

