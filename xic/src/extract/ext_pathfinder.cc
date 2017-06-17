
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
 $Id: ext_pathfinder.cc,v 5.80 2016/05/14 20:29:39 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_pathfinder.h"
#include "ext_grpgen.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "geo_ylist.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "errorlog.h"
#include "layertab.h"
#include "select.h"
#include "timer.h"
#include "timedbg.h"


namespace {
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


    inline bool isPathLayer(const CDl *ld)
    {
        return (EX()->isQuickPathUseConductor() ?
            ld->isConductor() : ld->isRouting());
    }
}


pf_lgen::pf_lgen(bool skipinv) : gen(DSP()->CurMode(),
    Selections.layerSearchUp() ?
        CDlgen::BotToTopNoCells : CDlgen::TopToBotNoCells)
{
    skip_inv = skipinv;
    gpld = 0;
    nogo = false;
    if (!LT()->CurLayer()) {
        nogo = true;
        return;
    }
}


// Iterator for the layer desc generator, return 0 when done.
//
CDl *
pf_lgen::next()
{
    if (nogo)
        return (0);
    CDl *ld = gen.next();
    if (!ld)
        return (0);
    while (ld) {
        if (!isPathLayer(ld)) {
            ld = gen.next();
            continue;
        }
        if (ld->isNoSelect()) {
            ld = gen.next();
            continue;
        }
        if (skip_inv && ld->isInvisible()) {
            ld = gen.next();
            continue;
        }
        if (ld->isGroundPlane()) {
            if (!ld->isDarkField())
                gpld = ld;
            ld = gen.next();
            continue;
        }
        return (ld);;
    }
    if (gpld) {
        CDl *tld = gpld;
        gpld = 0;
        return (tld);
    }
    return (0);
}
// End of pf_lgen functions.


pf_ordered_path::pf_ordered_path(pf_stack_elt *st)
{
    int cnt = 0;
    for (pf_stack_elt *s = st; s; s = s->next)
        cnt++;
    num_elements = cnt;
    if (cnt) {
        elements = new CDo*[cnt];
        int i = cnt - 1;
        for (pf_stack_elt *s = st; s; s = s->next) {
            elements[i] = s->odesc;
            i--;
        }
    }
    else
        elements = 0;
};


pf_ordered_path::pf_ordered_path(CDo *odesc)
{
    num_elements = 1;
    elements = new CDo*[1];
    elements[0] = odesc;
}


void
pf_ordered_path::simplify(pathfinder *pf)
{
    if (!(void*)this)
        return;
    for (int i = 0; i < num_elements; i++) {
        for (int j = num_elements - 1; j > i+1 ; j--) {
            if (pf->is_contacting(elements[i], elements[j])) {
                // remove i+1 ... j-1, connect i+1 to j
                int nr = j - (i+1);
                for (int k = j; k < num_elements; k++)
                    elements[k - nr] = elements[k];
                num_elements -= nr;
                break;
            }
        }
    }
}
// End of pf_ordered_path functions.


// Create a symbol table containing mutually connected objects.  The
// index is the "real" object desc, data are the copies as returned
// by the pseudo-flat generator.
//
bool
pathfinder::find_path(BBox *AOI)
{
    if (!AOI)
        return (false);
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (false);
    pf_topcell = cursdp->cellname();

    try {
        if (EX()->quickPathMode() != QPnone)
            EX()->activateGroundPlane(true);

        // find candidate object
        CDo *odesc = 0;
        pf_lgen ldgen(true);
        CDl *ld;
        while ((ld = ldgen.next()) != 0 && !odesc) {
            for (int i = 0; i <= pf_depth && !odesc; i++) {
                sPF gen(cursdp, AOI, ld, i);
                CDo *pointer;
                while ((pointer = gen.next(false, false)) != 0) {
                    if (pointer->type() == CDLABEL || !pointer->is_normal()) {
                        delete pointer;
                        continue;
                    }
                    odesc = pointer;
                    break;
                }
            }
        }

        if (odesc) {
            PFtype pfret = insert(odesc);
            if (pfret == PFerror) {
                delete odesc;
                throw;
            }
            if (pfret == PFdup) {
                // "can't happen"
                delete odesc;
                odesc = 0;
            }
            else {
                Tdbg()->start_timing("find_path");
                CDol *sl = new CDol(odesc, 0);
                CDol *se = sl;
                int ncnt = 0;
                while (sl) {
                    try {
                        CDol *sx = neighbors(cursdp, sl->odesc);
                        ncnt++;
                        se->next = sx;
                        while (se->next)
                            se = se->next;
                        CDol *sf = sl;
                        sl = sl->next;
                        delete sf;
                    }
                    catch (int) {
                        sl->free();
                        throw;
                    }
                }
                Tdbg()->stop_timing("find_path");
                if (Tdbg()->is_active())
                    printf("  (calls %d allocated %d)\n", ncnt,
                        pf_tab->allocated());
            }
        }
        if (EX()->quickPathMode() != QPnone)
            EX()->activateGroundPlane(false);

        if (pf_tab) {
            char buf[256];
            int x = (AOI->left + AOI->right)/2;
            int y = (AOI->bottom + AOI->top)/2;
            sprintf(buf, "%.4f_%.4f", MICRONS(x), MICRONS(y));
            // Encode characters which shouldn't be used in GDSII
            // cell names.
            for (char *s = buf; *s; s++) {
                if (*s == '-')
                    *s = 'm';
                else if (*s == '.')
                    *s = 'p';
            }
            pf_pathname = lstring::copy(buf);
            if (EX()->pathFinder(cExt::PFget) == this)
                EX()->PopUpSelections(0, MODE_UPD);
        }
        return (pf_tab && pf_tab->allocated() > 0);
    }
    catch (int) {
        if (EX()->quickPathMode() != QPnone)
            EX()->activateGroundPlane(false);
        clear();
        return (false);
    }
}


// As above, but find all objects in the net containing odesc.  The
// passed odesc is a conducting object in the top-level cell.
//
bool
pathfinder::find_path(const CDo *odesc)
{
    if (!odesc)
        return (false);
    if (odesc->type() != CDBOX && odesc->type() != CDPOLYGON &&
            odesc->type() != CDWIRE)
        return (false);
    if (!odesc->is_normal())
        return (false);
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (false);
    pf_topcell = cursdp->cellname();

    try {
        if (EX()->quickPathMode() != QPnone)
            EX()->activateGroundPlane(true);

        CDo *ocopy = odesc->copyObject();
        if (ocopy) {
            PFtype pfret = insert(ocopy);
            if (pfret == PFerror) {
                throw;
            }
            if (pfret == PFdup) {
                delete ocopy;
                ocopy = 0;
            }
            else {
                Tdbg()->start_timing("find_path");
                CDol *sl = new CDol(ocopy, 0);
                CDol *se = sl;
                int ncnt = 0;
                while (sl) {
                    try {
                        CDol *sx = neighbors(cursdp, sl->odesc);
                        ncnt++;
                        se->next = sx;
                        while (se->next)
                            se = se->next;
                        CDol *sf = sl;
                        sl = sl->next;
                        delete sf;
                    }
                    catch (int) {
                        sl->free();
                        throw;
                    }
                }
                Tdbg()->stop_timing("find_path");
                if (Tdbg()->is_active())
                    printf("  (calls %d allocated %d)\n", ncnt,
                        pf_tab->allocated());
            }
        }
        if (EX()->quickPathMode() != QPnone)
            EX()->activateGroundPlane(false);

        if (pf_tab) {
            char buf[64];
            int x, y;
            if (odesc->type() == CDPOLYGON) {
                const Point *pts = ((CDpo*)odesc)->points();
                x = pts[0].x;
                y = pts[0].y;
            }
            else if (odesc->type() == CDWIRE) {
                const Point *pts = ((CDw*)odesc)->points();
                x = pts[0].x;
                y = pts[0].y;
            }
            else {
                x = odesc->oBB().left;
                y = odesc->oBB().bottom;
            }
            sprintf(buf, "%.4f_%.4f", MICRONS(x), MICRONS(y));
            // Encode characters which shouldn't be used in GDSII
            // cell names.
            for (char *s = buf; *s; s++) {
                if (*s == '-')
                    *s = 'm';
                else if (*s == '.')
                    *s = 'p';
            }
            sprintf(buf, "%d", odesc->group());
            pf_pathname = lstring::copy(buf);
            if (EX()->pathFinder(cExt::PFget) == this)
                EX()->PopUpSelections(0, MODE_UPD);
        }
        return (pf_tab && pf_tab->allocated() > 0);
    }
    catch (int) {
        if (EX()->quickPathMode() != QPnone)
            EX()->activateGroundPlane(false);
        clear();
        return (false);
    }
}


// Reset and clear everything.
//
void
pathfinder::clear()
{
    delete [] pf_pathname;
    pf_pathname = 0;
    if (pf_tab) {
        SymTabGen gen(pf_tab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            CDo *on;
            for (CDo *od = (CDo*)h->stData; od; od = on) {
                on = od->next_odesc();
                if (od->is_copy())
                    // just a sanity test.
                    delete od;
            }
            delete h;
        }
        delete pf_tab;
        pf_tab = 0;
        if (EX()->pathFinder(cExt::PFget) == this)
            EX()->PopUpSelections(0, MODE_UPD);
    }
}
// End of virtual functions.


namespace {
    inline pf_ordered_path *esp_error(pathfinder *p)
    {
        Errs()->add_error("Internal error!");
        p->clear();
        return (0);
    }
}


// Return an orderd path containing the objects containing p1 and p2.
// The elements in pf_tab are unchanged.
//
pf_ordered_path *
pathfinder::extract_subpath(const BBox *BB1, const BBox *BB2)
{
    // Find the element that intersects BB1, remove it from the table
    // and save in odesc.

    CDo *odesc = find_and_remove(BB1);
    if (!odesc) {
        int ndgt = CD()->numDigits();
        Errs()->add_error(
            "extract_subpath: no path object at %.*f,%.*f %.*f,%.*f",
            ndgt, MICRONS(BB1->left), ndgt, MICRONS(BB1->bottom),
            ndgt, MICRONS(BB1->right), ndgt, MICRONS(BB1->top));
        return (0);
    }

    int depth = 1;
    CDol *trash = 0;

    // Check if this is also the terminal element.
    if (odesc->intersect(BB2, true)) {
        // We're done!
        pf_ordered_path *path = new pf_ordered_path(odesc);
        if (insert(odesc) == PFerror)
            return (esp_error(this));
        return (path);
    }

    // Find the element that intersects BB2, and save a pointer to it
    // in oterm.  This one is not removed from the table.

    CDo *oterm = find_and_remove(BB2);
    if (!oterm) {
        int ndgt = CD()->numDigits();
        Errs()->add_error(
            "extract_subpath: no path object at %.*f,%.*f %.*f,%.*f",
            ndgt, MICRONS(BB2->left), ndgt, MICRONS(BB2->bottom),
            ndgt, MICRONS(BB2->right), ndgt, MICRONS(BB2->top));
        if (insert(odesc) == PFerror)
            return (esp_error(this));
        return (0);
    }
    if (insert(oterm) == PFerror)
        return (esp_error(this));

    // Look at the intersecting elements until we find the terminal
    // element.  When done, the simplified path elements will be in
    // the stack, which is used to recreate the table.

    pf_stack_elt *stack = new pf_stack_elt(pf_tab, odesc, 0);

again:
    while (stack) {
        SymTabEnt *h = stack->ent;
        if (!h || !stack->onext)
            h = stack->gen.next();
        while (h) {
            CDo *od = stack->onext;
            if (!od)
                od = (CDo*)h->stData;
            while (od) {
                stack->onext = od->next_odesc();
                if (is_contacting(od, stack->odesc)) {

                    // Found touching element, remove it.
                    if (stack->oprev)
                        stack->oprev->set_next_odesc(stack->onext);
                    else
                        h->stData = stack->onext;

                    for (pf_stack_elt *s = stack; s; s = s->next) {
                        if (s->oprev == od)
                             s->oprev = stack->oprev;
                        if (s->onext == od)
                             s->onext = stack->onext;
                    }

                    stack->ent = h;
                    stack = new pf_stack_elt(pf_tab, od, stack);
                    depth++;

                    // If this one is the terminal element, create the
                    // path table from the stack, and return.
                    if (od == oterm) {
                        pf_ordered_path *path = new pf_ordered_path(stack);
                        path->simplify(this);

                        // Put everything back.
                        for (pf_stack_elt *s = stack; s; s = s->next) {
                            if (insert(s->odesc) == PFerror)
                                return (esp_error(this));
                        }
                        stack->free();
                        for (CDol *o = trash; o; o = o->next) {
                            if (insert(o->odesc) == PFerror)
                                return (esp_error(this));
                        }
                        trash->free();
                        return (path);
                    }
                    goto again;
                }
                stack->oprev = od;
                od = stack->onext;
            }
            stack->onext = stack->oprev = 0;
            h = stack->gen.next();
        }

        // Trash the present object, which is not connected to the
        // terminal object, and pop the stack.
        trash = new CDol(stack->odesc, trash);
        pf_stack_elt *tst = stack;
        stack = stack->next;
        delete tst;
        depth--;
    }

    // Failed to find a connecting subpath.  Put the trash back.
    for (CDol *o = trash; o; o = o->next) {
        if (insert(o->odesc) == PFerror)
            return (esp_error(this));
    }
    trash->free();
    Errs()->add_error("extract_subpath: no connected subpath found!");
    return (0);
}


// Load the ordered path into the table.
//
bool
pathfinder::load(pf_ordered_path *opath)
{
    if (!opath)
        return (false);
    // First, remove the elements from the table, so they won't be
    // deleted when the table is cleared.
    for (int i = 0; i < opath->num_elements; i++)
        remove(opath->elements[i]);
    clear();
    for (int i = 0; i < opath->num_elements; i++) {
        if (insert(opath->elements[i]) == PFerror)
            return (false);
    }
    EX()->PopUpSelections(0, MODE_UPD);
    return (true);
}


// Display or erase the current path.
//
void
pathfinder::show_path(WindowDesc *wdesc, bool d_or_e)
{
    if (!pf_tab || !pf_tab->allocated())
        return;
    if (wdesc) {
        if (!wdesc->Wdraw())
            return;
        if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
            return;

        static bool noenter;
        if (noenter)
            return;

        bool blink = EX()->isBlinkSelections();
        EX()->setBlinkSelections(false);

        if (d_or_e == ERASE) {
            if (!blink && dspPkgIf()->IsDualPlane())
                wdesc->Wdraw()->SetXOR(d_or_e ? GRxHlite : GRxUnhlite);
            else {
                BBox BB(CDnullBB);
                SymTabGen gen(pf_tab);
                SymTabEnt *h;
                while ((h = gen.next()) != 0) {
                    for (CDo *od = (CDo*)h->stData; od; od = od->next_odesc())
                        BB.add(&od->oBB());
                }
                noenter = true;
                wdesc->Refresh(&BB);
                noenter = false;
                EX()->setBlinkSelections(blink);
                return;
            }
        }
        else if (!pf_visible) {
            EX()->setBlinkSelections(blink);
            return;
        }

        if (blink)
            wdesc->Wdraw()->SetColor(DSP()->SelectPixel());
        else if (!dspPkgIf()->IsDualPlane())
            wdesc->Wdraw()->SetColor(
                DSP()->Color(HighlightingColor, Physical));

        SymTabGen gen(pf_tab);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            for (CDo *od = (CDo*)h->stData; od; od = od->next_odesc())
                wdesc->DisplaySelected(od);
        }

        if (!blink && dspPkgIf()->IsDualPlane())
            wdesc->Wdraw()->SetXOR(GRxNone);
        else if (LT()->CurLayer())
            wdesc->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
        EX()->setBlinkSelections(blink);
        return;
    }

    if (d_or_e == ERASE) {
        pf_visible = false;
        BBox BB(CDnullBB);
        SymTabGen gen(pf_tab);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            for (CDo *od = (CDo*)h->stData; od; od = od->next_odesc())
                BB.add(&od->oBB());
        }
        WindowDesc *wd;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0) {
            if (wd->IsSimilar(Physical, DSP()->MainWdesc()))
                wd->Refresh(&BB);
        }
    }
    else {
        pf_visible = true;
        WindowDesc *wd;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0)
            show_path(wd, DISPLAY);
    }
}


// Insert odesc into the table, unless it is a duplicate.
//
pathfinder::PFtype
pathfinder::insert(CDo *odesc)
{
    const char *ermsg =
        "Internal database inconsistency detected.\n%s\n"
        "Please report bug to Whiteley Research.";
    const char *erm1 = "insert: object is not a copy.";

    if (!odesc->is_copy()) {
        Log()->ErrorLogV(mh::Internal, ermsg, erm1);
        return (PFerror);
    }
    odesc->set_next_odesc(0);

    if (!pf_tab)
        pf_tab = new SymTab(false, false);

    unsigned int k = odesc->hash();
    // The hash value should be unique, however we account for the
    // possibility of non-unique hash values by using a linked list.

    SymTabEnt *ent = pf_tab->get_ent(k);
    if (!ent) {
        odesc->set_next_odesc(0);
        pf_tab->add(k, odesc, false);
        return (PFinserted);
    }
    CDo *ocopy = (CDo*)ent->stData;
    if (!ocopy)
        ent->stData = odesc; // shouldn't happen
    else {
        for (CDo *od = ocopy; od; od = od->next_odesc()) {
            if (od == odesc)
                // Already in list.
                return (PFdup);
            if (*od == *odesc)
                // Equivalent copy is already in list.
                return (PFdup);
            if (!od->next_odesc()) {
                od->set_next_odesc(odesc);
                break;
            }
        }
    }
    return (PFinserted);
}


// Remove ocopy from the table.  True is returned if the object was
// found and removed.
//
bool
pathfinder::remove(CDo *ocopy)
{
    if (!pf_tab)
        return (false);
    unsigned long tag = ocopy->hash();
    SymTabEnt *ent = pf_tab->get_ent(tag);
    if (!ent)
        // not found!
        return (false);
    CDo *op = 0, *on;
    for (CDo *od = (CDo*)ent->stData; od; od = on) {
        on = od->next_odesc();
        if (od == ocopy) {
            if (op)
                op->set_next_odesc(on);
            else {
                ent->stData = on;
                if (!on)
                    pf_tab->remove(tag);
            }
            od->set_next_odesc(0);
            return (true);
        }
        op = od;
    }
    // ocopy not found!
    return (false);
}


// If an object in the table overlaps BB, remove and return it (the
// first one found).
//
CDo *
pathfinder::find_and_remove(const BBox *BB)
{
    if (pf_tab) {
        SymTabGen gen(pf_tab);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            for (CDo *od = (CDo*)h->stData; od; od = od->next_odesc()) {
                if (od->intersect(BB, false)) {
                    if (remove(od))
                        return (od);
                }
            }
        }
    }
    return (0);
}


// Convert each polygon/wire path element into trapezoids.
//
bool
pathfinder::atomize_path()
{
    if (!pf_tab)
        return (false);

    CDo *o0 = 0;
    SymTabGen gen(pf_tab, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        CDo *od = (CDo*)h->stData;
        if (od) {
            CDo *ox = od;
            while (ox->next_odesc())
                ox = ox->next_odesc();
            ox->set_next_odesc(o0);
            o0 = od;
        }
        delete h;
    }
    while (o0) {
        CDo *ox = o0;
        o0 = o0->next_odesc();
        ox->set_next_odesc(0);
        if (ox->type() == CDBOX) {
            PFtype ret = insert(ox);
            if (ret == PFdup)
                delete ox;
            else if (ret == PFerror) {
                delete ox;
                while (o0) {
                    ox = o0;
                    o0 = o0->next_odesc();
                    delete ox;
                }
                return (false);
            }
        }
        else {
            Zlist *zl = ox->toZlist();
            CDl *ld = ox->ldesc();
            delete ox;
            if (zl) {
                CDo *odnew = zl->to_obj_list(ld, true);
                while (odnew) {
                    ox = odnew;
                    odnew = odnew->next_odesc();
                    ox->set_next_odesc(0);
                    PFtype ret = insert(ox);
                    if (ret == PFdup)
                        delete ox;
                    else if (ret == PFerror) {
                        delete ox;
                        while (odnew) {
                            ox = odnew;
                            odnew = odnew->next_odesc();
                            delete ox;
                        }
                        while (o0) {
                            ox = o0;
                            o0 = o0->next_odesc();
                            delete ox;
                        }
                        return (false);
                    }
                }
            }
        }
    }
    return (true);
}


// Return true if the two odescs are directly connected, by touching
// or through a via.
//
bool
pathfinder::is_contacting(CDo *od1, CDo *od2)
{
    if (!od1->intersect(od2, true))
        return (false);
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (false);
    CDl *ldesc1 = od1->ldesc();
    CDl *ldesc2 = od2->ldesc();
    if (!isPathLayer(ldesc1) || !isPathLayer(ldesc2))
        return (false);
    if (ldesc1 == ldesc2)
        return (true);

    // Look for Contact connections.
    if (EX()->quickPathMode() != QPnone && EX()->groundPlaneLayerInv()) {
        if (ldesc1 == EX()->groundPlaneLayerInv())
            ldesc1 = EX()->groundPlaneLayer();
        if (ldesc2 == EX()->groundPlaneLayerInv())
            ldesc2 = EX()->groundPlaneLayer();
    }
    if (ldesc1->isInContact()) {
        for (sVia *via = tech_prm(ldesc1)->via_list(); via; via = via->next()) {
            if (via->layer1() == ldesc2) {

                bool istrue = !via->tree();
                if (!istrue) {
                    sLspec lsp;
                    lsp.set_tree(via->tree());
                    XIrt ret = lsp.testContact(cursdp, CDMAXCALLDEPTH, od2,
                        &istrue);
                    lsp.set_tree(0);
                    if (ret == XIintr) {
                        throw (1);
                    }
                    if (ret == XIbad)
                        continue;
                }
                if (istrue)
                    return (true);
            }
        }
    }
    if (ldesc2->isInContact()) {
        for (sVia *via = tech_prm(ldesc2)->via_list(); via; via = via->next()) {
            if (via->layer1() == ldesc1) {

                bool istrue = !via->tree();
                if (!istrue) {
                    sLspec lsp;
                    lsp.set_tree(via->tree());
                    XIrt ret = lsp.testContact(cursdp, CDMAXCALLDEPTH, od1,
                        &istrue);
                    lsp.set_tree(0);
                    if (ret == XIintr) {
                        throw (1);
                    }
                    if (ret == XIbad)
                        continue;
                }
                if (istrue)
                    return (true);
            }
        }
    }

    // Look for connections through a via.
    CDextLgen lgen(CDL_VIA, CDextLgen::TopToBot);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {
            CDl *ld1 = via->layer1();
            CDl *ld2 = via->layer2();
            if (!ld1 || !ld2)
                continue;
            if (ld1->isInvisible() || ld2->isInvisible())
                continue;
            if ((ldesc1 == ld1 && ldesc2 == ld2) ||
                    (ldesc2 == ld1 && ldesc1 == ld2)) {

                sPF gen(cursdp, &od1->oBB(), ld, pf_depth);
                CDo *pointer;
                while ((pointer = gen.next(false, false)) != 0) {
                    if (!pointer->is_normal()) {
                        delete pointer;
                        continue;
                    }
                    if (checkInterrupt()) {
                        delete pointer;
                        throw (1);
                    }

                    bool istrue;
                    XIrt ret = cExt::isConnection(cursdp, via, pointer,
                        od1, od2, &istrue);
                    delete pointer;
                    if (ret == XIintr)
                        throw (1);
                    if (ret == XIbad)
                        continue;
                    if (istrue)
                        return (true);
                }
            }
        }
    }
    return (false);
}


// Return true if the current path is empty.
//
bool
pathfinder::is_empty()
{
    return (!pf_tab || !pf_tab->allocated());
}


// Return a list of copies of the objects that constitute the path.  The
// objects are linked through the next_odesc() pointer.
//
CDo *
pathfinder::get_object_list()
{
    if (!pf_tab || !pf_tab->allocated())
        return (0);
    SymTabGen gen(pf_tab);
    CDo *od0 = 0;
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        for (CDo *od = (CDo*)h->stData; od; od = od->next_odesc()) {
            CDo *ocpy = od->copyObject();
            ocpy->set_next_odesc(od0);
            od0 = ocpy;
        }
    }
    return (od0);
}


// Given a return from object_list, return a list of the needed via
// objects.  The objects are copies linked through the next_odesc()
// pointer.  The vias are obtained from the current physical cell,
// which must be consistent with the net objects passed.
//
// If incl_xtra_layers is given, the returned list will include layers
// required for the via tree (if any).  These objects are clipped to
// the via area.
//
CDo *
pathfinder::get_via_list(const CDo *od0, XIrt *err, bool incl_xtra_layers)
{
    *err = XIok;
    if (!od0)
        return (0);

    if (!pf_topcell) {
        *err = XIbad;
        Errs()->add_error(
            "get_via_list: internal error, null topcell pointer");
        return (0);
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        *err = XIbad;
        Errs()->add_error("get_via_list: no current physical cell");
        return (0);
    }
    if (cursdp->cellname() != pf_topcell) {
        *err = XIbad;
        Errs()->add_error(
            "get_via_list: current cell different from path source");
        return (0);
    }

    // Create a hash table of Zlists keyed by layer desc.
    //
    SymTab *tab = new SymTab(false, false);
    for (const CDo *od = od0; od; od = od->const_next_odesc()) {
        Zlist *zl = od->toZlist();
        if (!zl)
            continue;
        SymTabEnt *h = tab->get_ent((unsigned long)od->ldesc());
        if (!h) {
            tab->add((unsigned long)od->ldesc(), 0, false);
            h = tab->get_ent((unsigned long)od->ldesc());
        }
        if (!h->stData)
            h->stData = zl;
        else {
            Zlist *zx = zl;
            while (zx->next)
                zx = zx->next;
            zx->next = (Zlist*)h->stData;
            h->stData = zl;
        }
    }

    // Clip/merge the conductor lists.
    //
    SymTabGen stgen(tab);
    SymTabEnt *h;
    while ((h = stgen.next()) != 0)
        h->stData = ((Zlist*)h->stData)->repartition_ni();

    // Cycle through VIA layers.  For each via, find and save
    // via_layer & c1_layer & c2_layer.
    //
    XIrt ret = XIok;
    CDo *v0 = 0;
    CDl *ld;
    CDextLgen lgen(CDL_VIA, CDextLgen::TopToBot);
    while ((ld = lgen.next()) != 0) {
        Zlist *zv0 = 0;
        for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {
            CDl *ld1 = via->layer1();
            CDl *ld2 = via->layer2();
            if (!ld1 || !ld2)
                continue;

            Zlist *z1 = (Zlist*)tab->get((unsigned long)ld1);
            if (z1 == (Zlist*)ST_NIL)
                continue;
            Zlist *z2 = (Zlist*)tab->get((unsigned long)ld2);
            if (z2 == (Zlist*)ST_NIL)
                continue;
            z1 = z1->copy();
            z2 = z2->copy();

            ret = Zlist::zl_and(&z1, z2);
            if (ret != XIok) {
                if (ret == XIbad)
                    Errs()->add_error(
                        "get_via_list: clipping function returned error");
                break;
            }
            if (!z1)
                continue;

            Zlist *zv = cursdp->getZlist(pf_depth, ld, z1, &ret);
            z1->free();
            if (ret != XIok) {
                if (ret == XIbad)
                    Errs()->add_error(
                        "get_via_list: failed to get zlist for %s",
                        ld->name());
                break;
            }
            if (zv) {
                bool istrue = !via->tree();
                if (!istrue) {
                    sLspec lsp;
                    lsp.set_tree(via->tree());
                    ret = lsp.testContact(cursdp, pf_depth, zv, &istrue);
                    lsp.set_tree(0);
                    if (ret != XIok) {
                        if (ret == XIbad)
                            Errs()->add_error(
                                "get_via_list: via check returned error");
                        zv->free();
                        break;
                    }
                    if (istrue && incl_xtra_layers) {
                        CDll *l0 = via->tree()->findLayersInTree();
                        for (CDll *l = l0; l; l = l->next) {
                            CDl *ldtmp = l->ldesc;
                            Zlist *zx = cursdp->getZlist(pf_depth, ldtmp, zv,
                                &ret);
                            if (ret != XIok) {
                                if (ret == XIbad)
                                    Errs()->add_error(
                                    "get_via_list: failed to get zlist for %s",
                                        ldtmp->name());
                                zv->free();
                                break;
                            }
                            if (zx) {
                                PolyList *pl = zx->to_poly_list();
                                CDo *od = pl->to_odesc(ldtmp);
                                CDo *on;
                                for ( ; od; od = on) {
                                    on = od->next_odesc();
                                    od->set_next_odesc(v0);
                                    v0 = od;
                                }
                            }
                        }
                        l0->free();
                        if (ret != XIok)
                            break;
                    }
                }
                if (istrue) {
                    Zlist *zvt = zv;
                    while (zvt->next)
                        zvt = zvt->next;
                    zvt->next = zv0;
                    zv0 = zv;
                }
                else
                    zv->free();
            }
        }
        if (ret != XIok) {
            zv0->free();
            break;
        }

        // Subtlety here: the clip/merge in toPolyList should eliminate
        // coincident via objects generated from multiple Via lines on
        // a layer.

        PolyList *po = zv0->to_poly_list();
        CDo *od = po->to_odesc(ld);

        CDo *on;
        for ( ; od; od = on) {
            on = od->next_odesc();
            od->set_next_odesc(v0);
            v0 = od;
        }
    }

    // Clear the table.
    //
    SymTabGen gen(tab, true);
    while ((h = gen.next()) != 0) {
        ((Zlist*)h->stData)->free();
        delete h;
    }
    delete tab;

    if (ret != XIok) {
        while (v0) {
            CDo *vx = v0;
            v0 = v0->next_odesc();
            delete vx;
        }
    }
    *err = ret;
    return (v0);
}


// This function returns a list of objects which overlap the given
// object and are connected to it.  The elements are copies of the
// actual odescs as returned from the pseudo-flat generator.  If the
// user interrupts, this will return 0, with *interrupt set.
//
CDol *
pathfinder::neighbors(CDs *sdesc, CDo *odesc)
{
    CDol *sl0 = 0;
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (0);

    // First find touching objects on the same layer.
    {
        sPF gen(sdesc, &odesc->oBB(), odesc->ldesc(), pf_depth);
        CDo *pointer;
        while ((pointer = gen.next(false, true)) != 0) {
            if (pointer->type() == CDLABEL || !pointer->is_normal()) {
                // Should really check if pointer is a copy of odesc,
                // but this will be caught in insert().
                //
                delete pointer;
                continue;
            }
            if (checkInterrupt()) {
                sl0->free();
                delete pointer;
                throw (1);
            }
            if (!odesc->intersect(pointer, true)) {
                delete pointer;
                continue;
            }
            PFtype pfret = insert(pointer);
            if (pfret == PFdup) {
                delete pointer;
                continue;
            }
            if (pfret == PFerror) {
                sl0->free();
                delete pointer;
                return (0);
            }
            sl0 = new CDol(pointer, sl0);
        }
    }

    // Next, look for Contact connections.
    CDl *ld;
    CDextLgen lgen(CDL_IN_CONTACT, CDextLgen::TopToBot);
    while ((ld = lgen.next()) != 0) {
        for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {
            CDl *ld1 = via->layer1();
            if (!ld1)
                continue;
            if (ld1->isInvisible())
                continue;
            if (EX()->quickPathMode() != QPnone &&
                    EX()->groundPlaneLayerInv() &&
                    ld1 == EX()->groundPlaneLayer())
                ld1 = EX()->groundPlaneLayerInv();
            if (odesc->ldesc() != ld && odesc->ldesc() != ld1)
                continue;
            if (odesc->ldesc() == ld1)
                ld1 = ld;
            if (!isPathLayer(ld1))
                continue;

            sPF gen(sdesc, &odesc->oBB(), ld1, pf_depth);
            CDo *pointer;
            while ((pointer = gen.next(false, false)) != 0) {
                if (!pointer->is_normal()) {
                    delete pointer;
                    continue;
                }
                if (checkInterrupt()) {
                    sl0->free();
                    delete pointer;
                    throw (1);
                }
                if (!odesc->intersect(pointer, false)) {
                    delete pointer;
                    continue;
                }

                bool istrue = !via->tree();
                if (!istrue) {
                    sLspec lsp;
                    lsp.set_tree(via->tree());
                    XIrt ret = lsp.testContact(cursdp, CDMAXCALLDEPTH, pointer,
                        &istrue);
                    lsp.set_tree(0);
                    if (ret == XIintr) {
                        sl0->free();
                        delete pointer;
                        throw (1);
                    }
                    if (ret == XIbad) {
                        delete pointer;
                        continue;
                    }
                }
                if (!istrue) {
                    delete pointer;
                    continue;
                }
                PFtype pfret = insert(pointer);
                if (pfret == PFdup) {
                    delete pointer;
                    continue;
                }
                if (pfret == PFerror) {
                    sl0->free();
                    delete pointer;
                    return (0);
                }
                sl0 = new CDol(pointer, sl0);
            }
        }
    }

    // Finally, look for connections through a via.
    lgen = CDextLgen(CDL_VIA, CDextLgen::TopToBot);
    while ((ld = lgen.next()) != 0) {
        for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {
            CDl *ld1 = via->layer1();
            CDl *ld2 = via->layer2();
            if (!ld1 || !ld2 || (ld1 == ld2))
                continue;
            if (ld1->isInvisible() || ld2->isInvisible())
                continue;
            if (EX()->quickPathMode() != QPnone &&
                    EX()->groundPlaneLayerInv()) {
                if (ld1 == EX()->groundPlaneLayer())
                    ld1 = EX()->groundPlaneLayerInv();
                if (ld2 == EX()->groundPlaneLayer())
                    ld2 = EX()->groundPlaneLayerInv();
            }
            if (!isPathLayer(ld1) || !isPathLayer(ld2))
                continue;

            if (odesc->ldesc() != ld1 && odesc->ldesc() != ld2)
                continue;
            if (odesc->ldesc() == ld2) {
                ld2 = ld1;
                ld1 = odesc->ldesc();
            }

            sPF gen2(sdesc, &odesc->oBB(), ld2, pf_depth);
            CDo *odesc2;
            while ((odesc2 = gen2.next(false, false)) != 0) {
                if (odesc2->type() == CDLABEL || !odesc2->is_normal()) {
                    delete odesc2;
                    continue;
                }
                if (checkInterrupt()) {
                    sl0->free();
                    delete odesc2;
                    throw (1);
                }
                BBox BB(mmMax(odesc->oBB().left, odesc2->oBB().left),
                    mmMax(odesc->oBB().bottom, odesc2->oBB().bottom),
                    mmMin(odesc->oBB().right, odesc2->oBB().right),
                    mmMin(odesc->oBB().top, odesc2->oBB().top));
                BB.bloat(-1);
                if (BB.right <= BB.left || BB.top <= BB.bottom)
                    continue;

                bool isconn = false;
                sPF gen(sdesc, &BB, ld, pf_depth);
                CDo *pointer;
                while ((pointer = gen.next(false, false)) != 0) {
                    if (!pointer->is_normal()) {
                        delete pointer;
                        continue;
                    }
                    if (checkInterrupt()) {
                        sl0->free();
                        delete pointer;
                        throw (1);
                    }
                    XIrt ret = cExt::isConnection(cursdp, via, pointer,
                        odesc, odesc2, &isconn);
                    delete pointer;
                    if (ret == XIintr)
                        throw (1);
                    if (ret == XIbad)
                        continue;
                    if (isconn)
                        break;
                }
                if (isconn) {
                    PFtype pfret = insert(odesc2);
                    if (pfret == PFdup) {
                        delete odesc2;
                        continue;
                    }
                    if (pfret == PFerror) {
                        sl0->free();
                        delete odesc2;
                        return (0);
                    }
                    sl0 = new CDol(odesc2, sl0);
                    continue;
                }
                delete odesc2;
            }
        }
    }
    return (sl0);
}


//---------------------------------------------------------------------

// Add all objects in the path to the table.  If an object in a subckt
// is clicked on, the algorithm recurses to the top, then descends the
// hierarchy saving the appropriate groups.  If while recursing up
// ground is found, rf_found_ground is set, and we have to reset this
// and recurse from the top.  The structs don't provide the links for
// recursing up through the ground group,
//
bool
grp_pathfinder::find_path(BBox *AOI)
{
    if (!AOI)
        return (false);

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (false);
    pf_topcell = cursdp->cellname();

    Tdbg()->start_timing("find_path");

    pf_found_ground = false;
    grp_find_path(cursdp, AOI, 0);
    if (pf_found_ground) {
        pf_found_ground = false;
        recurse_path(cursdp->groups(), 0, 0);
    }

    Tdbg()->stop_timing("find_path");

    if (pf_tab) {
        char buf[256];
        int x = (AOI->left + AOI->right)/2;
        int y = (AOI->bottom + AOI->top)/2;
        sprintf(buf, "%.4f_%.4f", MICRONS(x), MICRONS(y));
        // Encode characters which shouldn't be used in GDSII
        // cell names.
        for (char *s = buf; *s; s++) {
            if (*s == '-')
                *s = 'm';
            else if (*s == '.')
                *s = 'p';
        }
        pf_pathname = lstring::copy(buf);
        if (EX()->pathFinder(cExt::PFget) == this)
            EX()->PopUpSelections(0, MODE_UPD);
    }
    return (true);
}


// As above, but find all objects in the net containing odesc.  The
// passed odesc is a conducting object in the top-level cell.
//
bool
grp_pathfinder::find_path(const CDo *odesc)
{
    if (!odesc)
        return (false);
    if (odesc->type() != CDBOX && odesc->type() != CDPOLYGON &&
            odesc->type() != CDWIRE)
        return (false);
    if (!odesc->is_normal())
        return (false);
    if (odesc->group() < 0)
        return (false);

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (false);
    pf_topcell = cursdp->cellname();

    Tdbg()->start_timing("find_path");

    pf_found_ground = false;
    recurse_path(cursdp->groups(), odesc->group(), 0);

    Tdbg()->stop_timing("find_path");

    if (pf_tab) {
        char buf[32];
        sprintf(buf, "%d", odesc->group());
        pf_pathname = lstring::copy(buf);
        if (EX()->pathFinder(cExt::PFget) == this)
            EX()->PopUpSelections(0, MODE_UPD);
    }
    return (true);
}


// Descend recursively until a conducting object is found, then start
// adding the connected group objects
//
bool
grp_pathfinder::grp_find_path(CDs *sdesc, BBox *AOI, int depth)
{
    if (!sdesc)
        return (false);
    if (TFull())
        return (false);
    if (checkInterrupt())
        throw (1);
    pf_lgen ldgen(false);
    cGroupDesc *gd = sdesc->groups();
    if (!gd)
        return (false);
    CDl *ld;
    while ((ld = ldgen.next()) != 0) {
        sGrpGen gdesc;
        gdesc.init_gen(gd, ld, AOI, this);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->type() == CDLABEL)
                continue;
            if (!odesc->is_normal())
                continue;
            CDo *ocopy = odesc->copyObjectWithXform(this);
            bool over = ocopy->intersect(AOI, true);
            delete ocopy;
            if (over) {
                if (!depth)
                    recurse_path(sdesc->groups(), odesc->group(), depth);
                else
                    recurse_up(odesc->group(), depth);
                return (true);
            }
        }
    }
    if (depth < pf_depth) {
        CDg gdesc;
        TInitGen(sdesc, CellLayer(), AOI, &gdesc);
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDs *msdesc = cdesc->masterCell(true);
            TPush();
            unsigned int x1, x2, y1, y2;
            if (TOverlapInst(cdesc, AOI, &x1, &x2, &y1, &y2)) {
                CDap ap(cdesc);
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    pf_stack[depth].cdesc = cdesc;
                    pf_stack[depth].x = xyg.x;
                    pf_stack[depth].y = xyg.y;
                    try {
                        if (grp_find_path(msdesc, AOI, depth + 1)) {
                            TPop();
                            return (true);
                        }
                    }
                    catch (int) {
                        TPop();
                        throw;
                    }
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            TPop();
        }
    }
    return (false);
}


// Move up in the hierarchy as far as possible, along the same conductor.
// Then add the connected groups recursively through recurse_path().
//
void
grp_pathfinder::recurse_up(int grp, int depth)
{
    if (checkInterrupt())
        throw (1);
    if (grp == 0) {
        pf_found_ground = true;
        return;
    }
    if (depth > 0 && grp >= 0) {
        CDc *cdesc = pf_stack[depth - 1].cdesc;
        CDap ap(cdesc);
        int ix = pf_stack[depth - 1].x;
        int iy = pf_stack[depth - 1].y;
        CDs *parent = cdesc->parent();
        if (parent) {
            cGroupDesc *gd = parent->groups();
            if (gd) {
                sSubcInst *s = gd->find_subc(cdesc, ix, iy);
                if (s) {
                    for (sSubcContactInst *c = s->contacts(); c;
                            c = c->next()) {
                        if (c->subc_group() == grp) {
                            TPop();
                            try {
                                if (depth == 1)
                                    recurse_path(gd, c->parent_group(), 0);
                                else
                                    recurse_up(c->parent_group(), depth - 1);
                            }
                            catch (int) {
                                TPush();
                                throw;
                            }
                            TPush();
                            TApplyTransform(cdesc);
                            TPremultiply();
                            TTransMult(ix*ap.dx, iy*ap.dy);
                            return;
                        }
                    }
                    for (sSubcContactInst *c = s->global_contacts(); c;
                            c = c->next()) {
                        if (c->subc_group() == grp) {
                            TPop();
                            try {
                                if (depth == 1)
                                    recurse_path(gd, c->parent_group(), 0);
                                else
                                    recurse_up(c->parent_group(), depth - 1);
                            }
                            catch (int) {
                                TPush();
                                throw;
                            }
                            TPush();
                            TApplyTransform(cdesc);
                            TPremultiply();
                            TTransMult(ix*ap.dx, iy*ap.dy);
                            return;
                        }
                    }
                }
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
grp_pathfinder::recurse_path(cGroupDesc *gdesc, int grp, int depth)
{
    if (!gdesc || grp < 0 || grp >= gdesc->num_groups())
        return;
    if (pf_found_ground)
        return;
    if (TFull())
        return;
    if (checkInterrupt())
        throw (1);
    if (grp == 0) {
        for (const sSubcList *sl = gdesc->subckts(); sl; sl = sl->next()) {
            for (sSubcInst *s = sl->subs(); s; s = s->next()) {
                CDc *cdesc = s->cdesc();
                CDs *sdesc = cdesc->masterCell(true);
                cGroupDesc *gd = sdesc->groups();
                if (!gd)
                    continue;
                TPush();
                TApplyTransform(cdesc);
                TPremultiply();
                CDap ap(cdesc);
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
                for (sSubcContactInst *c = s->contacts(); c; c = c->next()) {
                    if (c->parent_group() == grp) {
                        CDc *cdesc = s->cdesc();
                        CDs *sdesc = cdesc->masterCell(true);
                        cGroupDesc *gd = sdesc->groups();
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
                for (sSubcContactInst *c = s->global_contacts(); c;
                        c = c->next()) {
                    if (c->parent_group() == grp) {
                        CDc *cdesc = s->cdesc();
                        CDs *sdesc = cdesc->masterCell(true);
                        cGroupDesc *gd = sdesc->groups();
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
    sGroupObjs *go = gdesc->net_of_group(grp);
    if (go) {
        for (CDol *o = go->objlist(); o; o = o->next) {
            CDo *ocopy = o->odesc->copyObjectWithXform(this);
            // Force "real object" pointer to be the copied object.
            ocopy->set_next_odesc(o->odesc);
            if (insert(ocopy) == PFerror)
                return;
        }
    }
}

