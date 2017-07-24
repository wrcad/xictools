
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
 $Id: ext_connect.cc,v 5.12 2016/05/14 20:29:38 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_grpgen.h"
#include "ext_ufb.h"
#include "cd_lgen.h"
#include "tech_layer.h" 
#include "tech_via.h" 
#include "geo_zlist.h"   
#include "si_lexpr.h"

#include <algorithm>

#define TIME_DBG
#ifdef TIME_DBG
#include "timedbg.h"
#endif


int sGroupXf::gx_threshold = EXT_GRP_DEF_THRESH;

// Connect the groups in the main cell to those in the subcell hierarchy.
//
XIrt
cGroupDesc::connect_to_subs(const sSubcGen *cgx)
{
    Ufb ufb;
    ufb.save("Connecting to subcircuits in %s...",
        Tstring(gd_celldesc->cellname()));
    ufb.print();

    cTfmStack stk;
    for (int i = 0; i < gd_asize; i++) {
        sGroup &g1 = gd_groups[i];
        if (!g1.net())
            continue;

        BBox BB1(g1.net()->BB());

        ufb.save("Connecting to subcircuits in %s...  %d/%d",
            Tstring(gd_celldesc->cellname()), i, gd_asize);

        sGroupXf *gx1 = 0;
        bool skipsubs = false;
        sSubcGen cg2(*cgx);
        CDc *cdesc2;
        for ( ; (cdesc2 = cg2.current()) != 0; cg2.advance(skipsubs)) {
            if (ufb.checkPrint()) {
                delete gx1;
                return (XIintr);
            }
            skipsubs = false;
            CDs *sdesc2 = cdesc2->masterCell(true);
            if (!sdesc2)
                continue;

            BBox BB2(*sdesc2->BB());
            stk.TLoadCurrent(cg2.transform());
            stk.TBB(&BB2, 0);

            if (!BB1.intersect(&BB2, true)) {
                skipsubs = true;
                continue;
            }

            cGroupDesc *gd2 = sdesc2->groups();
            if (!gd2)
                continue;
            for (int j = 0; j < gd2->gd_asize; j++) {
                if (ufb.checkPrint()) {
                    delete gx1;
                    return (XIintr);
                }

                sGroup &g2 = gd2->gd_groups[j];
                if (!g2.net())
                    continue;

                BBox tBB(g2.net()->BB());
                stk.TBB(&tBB, 0);
                if (!BB1.intersect(&tBB, true))
                    continue;

                bool istrue;
                if (!gx1)
                    gx1 = new sGroupXf(gd_celldesc, g1.net());
                XIrt ret = gx1->connected(g2.net()->objlist(), &stk,
                    &istrue, 0);
                if (ret != XIok) {
                    delete gx1;
                    return (XIintr);
                }
                // Reflected ground connections are handled later.
                if (istrue)
                    add(i, &cg2, j);
            }
        }
        delete gx1;
    }

    // Check for the case where an above-the-cell via completes a
    // connection between two of the subcell's groups.  Add a virtual
    // group in this case.

    {
        bool skipsubs = false;
        sSubcGen cg(*cgx);
        CDc *cdesc1;
        for ( ; (cdesc1 = cg.current()) != 0; cg.advance(skipsubs)) {
            CDs *sdesc1 = cdesc1->masterCell(true);
            if (!sdesc1)
                continue;
            cGroupDesc *gd = sdesc1->groups();
            if (!gd)
                continue;

            // Unless there is a via over the subcell, we can skip it.
            skipsubs = false;
            bool found_via = false;
            CDl *ld;
            CDextLgen lgen(CDL_VIA, CDextLgen::TopToBot);
            while ((ld = lgen.next()) != 0) {
                CDg gdesc;
                gdesc.init_gen(gd_celldesc, ld, &cdesc1->oBB());
                if (gdesc.next()) {
                    found_via = true;
                    break;
                }
            }
            if (!found_via) {
                skipsubs = true;
                continue;
            }

            CDtf *tf1 = cg.transform();

            if (ufb.checkPrint())
                return (XIintr);

            for (int i = 0; i < gd->gd_asize; i++) {
                sGroup &gi = gd->gd_groups[i];
                if (!gi.net())
                    continue;

                sGroupXf *gxi = 0;
                for (int j = i+1; j < gd->gd_asize; j++) {
                    sGroup &gj = gd->gd_groups[j];
                    if (!gj.net())
                        continue;
                    if (!gi.net()->BB().intersect(&gj.net()->BB(), true))
                        continue;

                    bool istrue;
                    if (!gxi) {
                        gxi = new sGroupXf(gd_celldesc, gi.net());
                        gxi->TLoadCurrent(tf1);
                    }
                    XIrt ret = gxi->connected_by_via(gj.net()->objlist(),
                        0, &istrue);

                    if (ret != XIok) {
                        delete gxi;
                        return (XIintr);
                    }
                    if (istrue) {
                        const sSubcLink *c1 = cg.toplink();
                        if (i == 0)
                            add(0, &cg, j);
                        else {
                            int grp = vnextnum();
                            int n1 = add(grp, &cg, i);
                            int n2 = add(grp, &cg, j);
                            add_vcontact(c1->cdesc, c1->ix, c1->iy, n1,
                                c1->cdesc, c1->ix, c1->iy, n2);
                        }
                    }
                }
                delete gxi;
            }
        }
    }

    // Make sure that subcells that contain a ground group are added
    // to the subckts list.  This ensures that, e.g., all ground pads
    // will be included in the ground group, even if there is no other
    // connection.  Otherwise, they might be missing in the group
    // listing, which can be confusing.
    {
        sSubcGen cg(*cgx);
        CDc *cdesc;
        for ( ; (cdesc = cg.current()) != 0; cg.advance(true)) {
            CDs *sdesc = cdesc->masterCell(true);
            if (!sdesc)
                continue;
            cGroupDesc *gd = sdesc->groups();
            if (!gd)
                continue;
            if (gd->group_for(0))
                add(0, &cg, 0);
        }
    }
    return (XIok);
}


XIrt
cGroupDesc::connect_between_subs(const sSubcGen *cgx)
{
    Ufb ufb;
    ufb.save("Connecting between subcircuits in %s...",
        Tstring(gd_celldesc->cellname()));
    ufb.print();

    cTfmStack stk;
    sSubcGen cg1(*cgx);
    CDc *cdesc1;
    for ( ; (cdesc1 = cg1.current()) != 0; cg1.advance(true)) {
        if (ufb.checkPrint())
            return (XIintr);

        CDs *sdesc1 = cdesc1->masterCell(true);
        if (!sdesc1)
            continue;
        cGroupDesc *gd1 = sdesc1->groups();
        if (!gd1)
            continue;

        BBox BB1(*sdesc1->BB());
        stk.TLoadCurrent(cg1.transform());
        stk.TBB(&BB1, 0);

        sSubcGen cg2(cg1);
        cg2.advance_top();
        CDc *cdesc2;
        for ( ; (cdesc2 = cg2.current()) != 0; cg2.advance(true)) {
            if (ufb.checkPrint())
                return (XIintr);

            CDs *sdesc2 = cdesc2->masterCell(true);
            if (!sdesc2)
                continue;

            BBox BB2(*sdesc2->BB());
            stk.TLoadCurrent(cg2.transform());
            stk.TBB(&BB2, 0);

            BBox ovlBB(BB1, BB2);
            if (ovlBB.width() >= 0 && ovlBB.height() >= 0) {
                cGroupDesc *gd2 = sdesc2->groups();
                if (!gd2)
                    continue;

                XIrt ret;
                if (gd2->gd_asize > gd1->gd_asize) {
                    ret = connect_subs(&cg2, &cg1, ovlBB, ufb, false);
                    if (ret != XIok)
                        return (ret);
                    ret = connect_subs(&cg1, &cg2, ovlBB, ufb, true);
                }
                else {
                    ret = connect_subs(&cg1, &cg2, ovlBB, ufb, false);
                    if (ret != XIok)
                        return (ret);
                    ret = connect_subs(&cg2, &cg1, ovlBB, ufb, true);
                }
                if (ret != XIok)
                    return (ret);
            }
        }
    }
    return (XIok);
}


// Descend through the hierarchies of the two top subcells, establishing
// connections between nets.  The top cells are known to touch.
//
XIrt
cGroupDesc::connect_subs(const sSubcGen *cgx, const sSubcGen *cgy,
    const BBox &ovlBB, Ufb &ufb, bool pass2)
{
    sSubcGen cg1(*cgx);
    const sSubcLink *tl1 = cg1.toplink();
    cTfmStack stk;
    CDc *cdesc1;
    for ( ; (cdesc1 = cg1.current()) != 0 && cg1.toplink() == tl1;
            cg1.advance(false)) {

        if (ufb.checkPrint())
            return (XIintr);

        CDs *sdesc1 = cdesc1->masterCell(true);
        if (!sdesc1)
            continue;
        cGroupDesc *gd1 = sdesc1->groups();
        if (!gd1)
            continue;

        // Note that we don't transform cg1, and use a composite
        // transform for cg2, which should be more efficient than
        // transforming both to top-level coords.  However, we have to
        // apply the transform before the search for vias, which is
        // done in top-level coords.

        CDtf *tf1 = cg1.transform();
        stk.TPush();
        stk.TLoadCurrent(tf1);
        stk.TInverse();
        stk.TLoadInverse();
        BBox ovBB(ovlBB);
        stk.TBB(&ovBB, 0);

        if (!ovBB.intersect(sdesc1->BB(), true)) {
            stk.TPop();
            continue;
        }

        for (int i = 0; i < gd1->gd_asize; i++) {
            sGroup &gi = gd1->gd_groups[i];
            if (!gi.net())
                continue;

            if (ufb.checkPrint())
                return (XIintr);

            BBox BB1(ovBB, gi.net()->BB());
            if (BB1.width() < 0 || BB1.height() < 0)
                continue;

            sGroupXf *gxi = 0;
            sSubcGen cg2(*cgy);
            const sSubcLink *tl2 = cg2.toplink();
            CDc *cdesc2;
            bool skipsubs = false;
            for ( ; (cdesc2 = cg2.current()) != 0 && cg2.toplink() == tl2;
                    cg2.advance(skipsubs)) {
                skipsubs = false;
                CDs *sdesc2 = cdesc2->masterCell(true);
                if (!sdesc2)
                    continue;
                cGroupDesc *gd2 = sdesc2->groups();
                if (!gd2)
                    continue;

                stk.TPush();
                stk.TLoadCurrent(cg2.transform());
                stk.TPremultiply();

                BBox BB2(*sdesc2->BB());
                stk.TBB(&BB2, 0);
                if (!BB1.intersect(&BB2, true)) {
                    skipsubs = true;
                    stk.TPop();
                    continue;
                }
                for (int j = (i == 0 ? 1 : 0); j < gd2->gd_asize; j++) {
                    sGroup &gj = gd2->gd_groups[j];
                    if (!gj.net())
                        continue;
                    if (gj.net()->length() > gi.net()->length())
                        continue;
                    if (pass2 && gj.net()->length() == gi.net()->length())
                        continue;
                    BBox tBB(gj.net()->BB());
                    stk.TBB(&tBB, 0);
                    if (!BB1.intersect(&tBB, true))
                        continue;

                    bool istrue;
                    if (!gxi) {
                        gxi = new sGroupXf(gd_celldesc, gi.net());
                        gxi->TLoadCurrent(tf1);
                    }
                    XIrt ret = gxi->connected(gj.net()->objlist(), &stk,
                        &istrue, 1);

                    if (ret != XIok) {
                        delete gxi;
                        return (XIintr);
                    }
                    if (istrue) {
                        const sSubcLink *c1 = cg1.toplink();
                        const sSubcLink *c2 = cg2.toplink();
                        if (i == 0) {
                            add(0, &cg1, 0);
                            add(0, &cg2, j);
                        }
                        else if (j == 0) {
                            add(0, &cg1, i);
                            add(0, &cg2, 0);
                        }
                        else {
                            int grp = vnextnum();
                            int n1 = add(grp, &cg1, i);
                            int n2 = add(grp, &cg2, j);
                            add_vcontact(c1->cdesc, c1->ix, c1->iy, n1,
                                c2->cdesc, c2->ix, c2->iy, n2);
                        }
                    }
                }
                stk.TPop();
            }
            delete gxi;
            cg2.clear();
        }
        stk.TPop();
    }
    return (XIok);
}
// End of cGroupDesc functions.


// Return the total area of the objects in the net.  This takes no
// account of overlap.
//
double
sGroupObjs::area() const
{
    double a = 0;
    for (CDol *o = objlist(); o; o = o->next)
        a += o->odesc->area();
    return (a);
}


// Return a list element containing the first object found that
// touches tBB.  The list will be sorted if necessary.
//
CDol *
sGroupObjs::find_object(const CDl *ld, const BBox *tBB)
{
    sort();

    // Find a suitable object.
    CDol *ol0 = 0;
    for (CDol *o = objlist(); o; o = o->next) {
        CDo *od = o->odesc;
        if (od->ldesc() != ld)
            continue;
        if (od->oBB().bottom > tBB->top)
            continue;
        if (od->oBB().top < tBB->bottom)
            return (0);
        if (od->intersect(tBB, true)) {
            ol0 = new CDol(od, 0);
            break;
        }
    }
    return (ol0);
}


// Add zoids to ol0 that touch (recursively) an element in ol0, and are
// not in the hash table, which contains the objects in ol0 as passed.
//
void
sGroupObjs::accumulate(CDol *ol0, SymTab *tab)
{
    if (!ol0 || !tab)
        return;
    sort();
    CDl *ld = ol0->odesc->ldesc();

    // Create sorted list of other objects on layer.
    CDol *ol1 = 0, *oe = 0;
    int cnt = 0;
    for (CDol *o = objlist(); o; o = o->next) {
        CDo *od = o->odesc;
        if (od->ldesc() == ld && SymTab::get(tab, (unsigned long)od) ==
                ST_NIL) {
            if (!ol1)
                ol1 = oe = new CDol(od, 0);
            else {
                oe->next = new CDol(od, 0);
                oe = oe->next;
            }
            cnt++;
        }
    }
    if (!cnt)
        return;

    // Pull connected objects onto ol0.
    cnt = 0;
    oe = ol0;
    while (oe->next)
        oe = oe->next;
    for (CDol *oc = ol0; oc; oc = oc->next) {
        CDo *od0 = oc->odesc;
        CDol *op = 0, *on;
        for (CDol *o = ol1; o; o = on) {
            on = o->next;
            CDo *od1 = o->odesc;
            if (od1->oBB().bottom > od0->oBB().top) {
                op = o;
                continue;
            }
            if (od1->oBB().top < od0->oBB().bottom)
                break;
            if (od0->intersect(od1, true)) {
                if (!op)
                    ol1 = on;
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
    CDol::destroy(ol1);
}


// Return a list of group objects on ld, that touch tBB.
//
CDol *
sGroupObjs::accumulate(const CDl *ld, const BBox *tBB)
{
    CDol *ol0 = find_object(ld, tBB);
    if (!ol0)
        return (0);

    // Create sorted list of other objects on layer.
    CDol *ol1 = 0, *oe = 0;
    int cnt = 0;
    for (CDol *o = objlist(); o; o = o->next) {
        CDo *od = o->odesc;
        if (od->ldesc() == ld && od != ol0->odesc) {
            if (!ol1)
                ol1 = oe = new CDol(od, 0);
            else {
                oe->next = new CDol(od, 0);
                oe = oe->next;
            }
            cnt++;
        }
    }
    if (!cnt)
        return (ol0);

    // Pull connected objects onto ol0.
    cnt = 0;
    oe = ol0;
    for (CDol *oc = ol0; oc; oc = oc->next) {
        CDo *od0 = oc->odesc;
        CDol *op = 0, *on;
        for (CDol *o = ol1; o; o = on) {
            on = o->next;
            CDo *od1 = o->odesc;
            if (od1->oBB().bottom > od0->oBB().top) {
                op = o;
                continue;
            }
            if (od1->oBB().top < od0->oBB().bottom)
                break;
            if (od0->intersect(od1, true)) {
                if (!op)
                    ol1 = on;
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
    CDol::destroy(ol1);
    return (ol0);
}


// Return true if some object in the net list intersects odesc, on the
// same layer.
//
bool
sGroupObjs::intersect(const CDo *odesc, bool touchok) const
{
    if (odesc->intersect(&go_BB, touchok)) {
        for (CDol *ol = objlist(); ol; ol = ol->next) {
            if (ol->odesc->ldesc() != odesc->ldesc())
                continue;
            if (odesc->intersect(ol->odesc, true))
                return (true);
        }
    }
    return (false);
}


// Return true if some object in the net intersects AOI.
//
bool
sGroupObjs::intersect(const BBox *AOI, bool touchok) const
{
    if (AOI->intersect(&go_BB, touchok)) {
        for (CDol *ol = objlist(); ol; ol = ol->next) {
            if (ol->odesc->intersect(AOI, true))
                return (true);
        }
    }
    return (false);
}


// Return true if there is connectivity due to proximity to the g2
// net.  Side effect:  the net is sorted and the flag is set.
//
bool
sGroupObjs::intersect(const sGroupObjs &go2)
{
    if (!objlist() || !go2.objlist())
        return (false);
    if (!go_BB.intersect(&go2.go_BB, true))
        return (false);

    sort();

    for (CDol *o1 = go2.objlist(); o1; o1 = o1->next) {
        for (CDol *o2 = objlist(); o2; o2 = o2->next) {
            if (o2->odesc->oBB().bottom > o1->odesc->oBB().top)
                continue;
            if (o2->odesc->oBB().top < o1->odesc->oBB().bottom)
                break;
            if (o1->odesc->ldesc() != o2->odesc->ldesc())
                continue;
            if (o1->odesc->intersect(o2->odesc, true))
                return (true);
        }
    }
    return (false);
}


// Create a dummy cell conntaining transformed copies of the group
// objects.  We then have fast spatial access to the group elements.
//
CDs *
sGroupObjs::mk_cell(const CDtf *tf, const CDl *ld) const
{
    if (!objlist())
        return (0);

    CDs *sd = new CDs(0, Physical);
    if (tf) {
        cTfmStack stk;
        stk.TPush();
        stk.TLoadCurrent(tf);
        for (CDol *o = objlist(); o; o = o->next) {
            CDo *odesc = o->odesc;
            if (ld && ld != odesc->ldesc())
                continue;
            if (odesc->type() == CDBOX) {
                BBox bBB(odesc->oBB());
                stk.TBB(&bBB, 0);
                CDo *newb = new CDo(odesc->ldesc(), &bBB);
                sd->insert(newb);
            }
            else if (odesc->type() == CDPOLYGON) {
                CDpo *podesc = (CDpo*)odesc;
                Poly po;
                po.numpts = podesc->numpts();
                po.points = Point::dup_with_xform(podesc->points(), &stk,
                    podesc->numpts());
                CDpo *newp = new CDpo(odesc->ldesc(), &po);
                sd->insert(newp);
            }
            else if (odesc->type() == CDWIRE) {
                CDw *wodesc = (CDw*)odesc;
                Wire w;
                w.numpts = wodesc->numpts();
                w.points = Point::dup_with_xform(wodesc->points(), &stk,
                    wodesc->numpts());
                w.set_wire_style(wodesc->wire_style());
                w.set_wire_width(wodesc->wire_width());
                CDw *neww = new CDw(odesc->ldesc(), &w);
                sd->insert(neww);
            }
        }
        stk.TPop();
    }
    else {
        for (CDol *o = objlist(); o; o = o->next) {
            CDo *odesc = o->odesc;
            if (ld && ld != odesc->ldesc())
                continue;
            if (odesc->type() == CDBOX) {
                CDo *newb = new CDo(odesc->ldesc(), &odesc->oBB());
                sd->insert(newb);
            }
            else if (odesc->type() == CDPOLYGON) {
                CDpo *podesc = (CDpo*)odesc;
                Poly po;
                po.numpts = podesc->numpts();
                po.points = Point::dup(podesc->points(), podesc->numpts());
                CDpo *newp = new CDpo(odesc->ldesc(), &po);
                sd->insert(newp);
            }
            else if (odesc->type() == CDWIRE) {
                CDw *wodesc = (CDw*)odesc;
                Wire w;
                w.numpts = wodesc->numpts();
                w.points = Point::dup(wodesc->points(), wodesc->numpts());
                w.set_wire_style(wodesc->wire_style());
                w.set_wire_width(wodesc->wire_width());
                CDw *neww = new CDw(odesc->ldesc(), &w);
                sd->insert(neww);
            }
        }
    }
    sd->computeBB();
    return (sd);
}


namespace {
    // Sort comparison for grouping, descending in top, ascending in
    // left.
    //
    inline bool
    gcmp(const CDo *o1, const CDo *o2)
    {
        const BBox *BB1 = &o1->oBB();
        const BBox *BB2 = &o2->oBB();
        if (BB1->top > BB2->top)
            return (true);
        if (BB1->top < BB2->top)
            return (false);
        return (BB1->left < BB2->left);
    }
}


// Static function.
// Sort descending in oBB().top, ascending in oBB().left.
//
void
sGroupObjs::sort_list(CDol *ol)
{
    int cnt = 0;
    for (CDol *o = ol; o; o = o->next, cnt++) ;
    if (cnt > 1) {
        CDo **zz = new CDo*[cnt];
        CDol *o0 = ol;
        for (int i = 0; i < cnt; i++) {
            zz[i] = o0->odesc;
            o0 = o0->next;
        }
        std::sort(zz, zz + cnt, gcmp);
        o0 = ol;
        for (int i = 0; i < cnt; i++) {
            o0->odesc = zz[i];
            o0 = o0->next;
        }
        delete [] zz;
    }
}
// End of sGroupObjs functions.


sGroupXf::sGroupXf(CDs *topc, sGroupObjs *net, const CDl *ld)
{
    gx_topcell = topc;
    gx_sdesc = 0;
    gx_objs = 0;
    if (!net)
        return;

    if (gx_threshold <= 0) {
        // Don't use cells.
        gx_objs = net;
        return;
    }
    // If gx_threshold objects or more, use a cell.
    int cnt = 0;
    for (CDol *o = net->objlist(); o; o = o->next) {
        cnt++;
        if (cnt == gx_threshold)
            break;
    }
    if (cnt == gx_threshold)
        gx_sdesc = net->mk_cell(0, ld);
    else
        gx_objs = net;
}


// Set istrue and return XIok if the net in gx1 electrically connects
// to the collection in ol2.  The tf1 transforms the gx1 objects to
// the top level coords.  The tf21 transforms the ol2 objects to the
// gx1 coordinates.
//
XIrt
sGroupXf::connected(CDol *ol2, const cTfmStack *stk21, bool *istrue,
    int state) const
{
#ifdef TIME_DBG
    Tdbg()->start_timing("direct_contact");
#endif

    // Test for direct contact.
    XIrt ret = connected_direct(ol2, stk21, istrue);

#ifdef TIME_DBG
    Tdbg()->accum_timing("direct_contact");
#endif

    if (ret != XIok || *istrue)
        return (ret);

    // Check for contact through Contact layers
    if (CDextLgen::ext_ltab()->num_contacts() > 0) {
#ifdef TIME_DBG
        Tdbg()->start_timing("layer_contact");
#endif
        ret = connected_by_contact(ol2, stk21, istrue);
#ifdef TIME_DBG
        Tdbg()->accum_timing("layer_contact");
#endif
        if (ret != XIok || *istrue)
            return (ret);
    }

    if (!state || EX()->isViaCheckBtwnSubs()) {
#ifdef TIME_DBG
        Tdbg()->start_timing("via_contact");
#endif

        ret = connected_by_via(ol2, stk21, istrue);

#ifdef TIME_DBG
        Tdbg()->accum_timing("via_contact");
#endif
    }

    return (ret);
}


// Core test for direct conntact of metal on the same layer between
// the two dummy cells.
//
XIrt
sGroupXf::connected_direct(CDol *ol2, const cTfmStack *stk21, bool *istrue)
    const
{
    *istrue = false;
    if (gx_objs) {
        gx_objs->sort();
        for (CDol *o2 = ol2; o2; o2 = o2->next) {
            CDo *od2 = o2->odesc;
            BBox BB2(od2->oBB());
            if (stk21)
                stk21->TBB(&BB2, 0);

            if (od2->type() == CDBOX) {
                for (CDol *o1 = gx_objs->objlist(); o1; o1 = o1->next) {
                    CDo *od1 = o1->odesc;
                    if (od1->oBB().bottom > BB2.top)
                        continue;
                    if (od1->oBB().top < BB2.bottom)
                        break;
                    if (od1->ldesc() != od2->ldesc())
                        continue;
                    if (od1->oBB().right < BB2.left ||
                            od1->oBB().left > BB2.right)
                        continue;
                    if (od1->type() == CDBOX || od1->intersect(&BB2, true)) {
                        *istrue = true;
                        return (XIok);
                    }
                }
            }
            else {
                CDo *od2t = 0;
                for (CDol *o1 = gx_objs->objlist(); o1; o1 = o1->next) {
                    CDo *od1 = o1->odesc;
                    if (od1->oBB().bottom > BB2.top)
                        continue;
                    if (od1->oBB().top < BB2.bottom)
                        break;
                    if (od1->ldesc() != od2->ldesc())
                        continue;
                    if (od1->oBB().right < BB2.left ||
                            od1->oBB().left > BB2.right)
                        continue;
                    if (!od2t)
                        od2t = od2->copyObjectWithXform(stk21);
                    if (od1->intersect(od2t, true)) {
                        delete od2t;
                        *istrue = true;
                        return (XIok);
                    }
                }
                delete od2t;
            }
        }
    }
    else if (gx_sdesc) {
        for (CDol *o2 = ol2; o2; o2 = o2->next) {
            CDo *od2 = o2->odesc;
            BBox BB2(od2->oBB());
            if (stk21)
                stk21->TBB(&BB2, 0);

            CDg gd1;
            gd1.init_gen(gx_sdesc, od2->ldesc(), &BB2);
            CDo *od1;
            if (od2->type() == CDBOX) {
                while ((od1 = gd1.next()) != 0) {
                    if (od1->type() == CDBOX || od1->intersect(&BB2, true)) {
                        *istrue = true;
                        return (XIok);
                    }
                }
            }
            else {
                CDo *od2t = 0;
                while ((od1 = gd1.next()) != 0) {
                    if (!od2t)
                        od2t = od2->copyObjectWithXform(stk21);
                    if (od1->intersect(od2t, true)) {
                        delete od2t;
                        *istrue = true;
                        return (XIok);
                    }
                }
                delete od2t;
            }
        }
    }
    return (XIok);
}


// Test for connection by contact layer.
//
XIrt
sGroupXf::connected_by_contact(CDol *ol2, const cTfmStack *stk21, bool *istrue)
    const
{
    *istrue = false;
    CDl *ldco;
    CDextLgen cgen(CDL_IN_CONTACT, CDextLgen::TopToBot);
    while ((ldco = cgen.next()) != 0) {
        for (sVia *via = tech_prm(ldco)->via_list(); via; via = via->next()) {
            CDl *ldm = via->layer1();
            if (!ldm)
                continue;
            if (EX()->groundPlaneLayerInv() && ldm == EX()->groundPlaneLayer())
                ldm = EX()->groundPlaneLayerInv();
            if (!ldm->isConductor())
                continue;

            XIrt ret = connected_by_contact(ldco, ol2, ldm, stk21, via,
                istrue);
            if (ret == XIok && !*istrue)
                ret = connected_by_contact(ldm, ol2, ldco, stk21, via,
                    istrue);
            if (ret != XIok || *istrue)
                return (ret);
        }
    }
    return (XIok);
}


// Core test for connection by contact layer.  This should be called
// twice, permuting ld1 and ld2.
//
XIrt
sGroupXf::connected_by_contact(const CDl *ld1, CDol *ol2, const CDl *ld2,
    const cTfmStack *stk21, const sVia *via, bool *istrue) const
{
    if (gx_objs) {
        gx_objs->sort();
        cTfmStack stk;
        for (CDol *o2 = ol2; o2; o2 = o2->next) {
            CDo *od2 = o2->odesc;
            if (od2->ldesc() != ld2)
                continue;
            BBox BB2(od2->oBB());
            if (stk21)
                stk21->TBB(&BB2, 0);

            CDo *od2t = 0;
            for (CDol *o1 = gx_objs->objlist(); o1; o1 = o1->next) {
                CDo *od1 = o1->odesc;
                if (od1->oBB().bottom >= BB2.top)
                    continue;
                if (od1->oBB().top <= BB2.bottom)
                    break;
                if (od1->ldesc() != ld1)
                    continue;
                if (od1->oBB().right <= BB2.left ||
                        od1->oBB().left >= BB2.right)
                    continue;

                if (!od2t)
                    od2t = od2->copyObjectWithXform(stk21);
                if (od1->intersect(od2t, false)) {

                    bool istr = !via->tree();
                    if (!istr) {
                        sLspec lsp;
                        lsp.set_tree(via->tree());
                        CDo *ocopy =  ld1->isInContact() ? od1 : od2t;
                        ocopy = ocopy->copyObjectWithXform(this);
                        XIrt xirt = lsp.testContact(gx_topcell, CDMAXCALLDEPTH,
                            ocopy, &istr);
                        delete ocopy;
                        lsp.set_tree(0);
                        if (xirt != XIok) {
                            delete od2t;
                            return (xirt);
                        }
                    }
                    if (istr) {
                        delete od2t;
                        *istrue = true;
                        return (XIok);
                    }
                }
            }
            delete od2t;
        }
    }
    else if (gx_sdesc) {
        for (CDol *ol = ol2; ol; ol = ol->next) {
            CDo *od2 = ol->odesc;
            if (od2->ldesc() != ld2)
                continue;
            BBox BB2(od2->oBB());
            if (stk21)
                stk21->TBB(&BB2, 0);

            CDo *od2t = 0;
            CDg gd1;
            gd1.init_gen(gx_sdesc, ld1, &BB2);
            CDo *od1;
            while ((od1 = gd1.next()) != 0) {

                if (!od2t)
                    od2t = od2->copyObjectWithXform(stk21);
                if (od1->intersect(od2t, false)) {

                    bool istr = !via->tree();
                    if (!istr) {
                        sLspec lsp;
                        lsp.set_tree(via->tree());
                        CDo *ocopy =  ld1->isInContact() ? od1 : od2t;
                        ocopy = ocopy->copyObjectWithXform(this);
                        XIrt xirt = lsp.testContact(gx_topcell, CDMAXCALLDEPTH,
                            ocopy, &istr);
                        delete ocopy;
                        lsp.set_tree(0);
                        if (xirt != XIok) {
                            delete od2t;
                            return (xirt);
                        }
                    }
                    if (istr) {
                        *istrue = true;
                        delete od2t;
                        return (XIok);
                    }
                }
            }
            delete od2t;
        }
    }
    return (XIok);
}


// Test for connection by via.
//
XIrt
sGroupXf::connected_by_via(CDol *ol2, const cTfmStack *stk21,
    bool *istrue) const
{
    *istrue = false;
    CDl *ld;
    CDextLgen vgen(CDL_VIA, CDextLgen::TopToBot);
    while ((ld = vgen.next()) != 0) {
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

            XIrt ret = connected_by_via(ld1, ol2, ld2, stk21, via, ld, istrue);
            if (ret == XIok && !*istrue)
                ret = connected_by_via(ld2, ol2, ld1, stk21, via, ld, istrue);
            if (ret != XIok || *istrue)
                return (ret);
        }
    }
    return (XIok);
}


namespace {
    struct Vcheck
    {
        Vcheck(const CDl *ld1, const CDl *ld2, const CDol *ol1,
            const CDol *ol2, const cTfmStack *stk21)
            {
                // Keep the order, the lists might be sorted.

                ol11 = 0;
                ol22 = 0;
                CDol *oe = 0;
                for (const CDol *o = ol1; o; o = o->next) {
                    if (o->odesc->ldesc() == ld1) {
                        if (!ol11) {
                            ol11 = oe = new CDol(o->odesc, 0);
                            BB11 = o->odesc->oBB();
                        }
                        else {
                            oe->next = new CDol(o->odesc, 0);
                            oe = oe->next;
                            BB11.add(&o->odesc->oBB());
                        }
                    }
                }
                if (!ol11)
                    return;
                oe = 0;
                for (const CDol *o = ol2; o; o = o->next) {
                    if (o->odesc->ldesc() == ld2) {
                        if (!ol22) {
                            ol22 = oe = new CDol(o->odesc, 0);
                            BB22 = o->odesc->oBB();
                        }
                        else {
                            oe->next = new CDol(o->odesc, 0);
                            oe = oe->next;
                            BB22.add(&o->odesc->oBB());
                        }
                    }
                }
                if (!ol22) {
                    CDol::destroy(ol11);
                    ol11 = 0;
                    return;
                }

                if (stk21)
                    stk21->TBB(&BB22, 0);
                BB1 = BBox(BB11, BB22);
                BB1.bloat(-1);
                if (BB1.right <= BB1.left || BB1.top <= BB1.bottom) {
                    CDol::destroy(ol11);
                    ol11 = 0;
                    CDol::destroy(ol22);
                    ol22 = 0;
                }
            }

        ~Vcheck()
            {
                CDol::destroy(ol11);
                CDol::destroy(ol22);
            }

        CDol *ol11;
        CDol *ol22;
        BBox BB11;
        BBox BB22;
        BBox BB1;
    };
}


// Core test for connect-by-via.  This should be called twice,
// permuting ld1 and ld2.
//
XIrt
sGroupXf::connected_by_via(const CDl *ld1, CDol *ol2, const CDl *ld2,
    const cTfmStack *stk21, const sVia *via, const CDl *ldv,
    bool *istrue) const
{
    int via_depth = EX()->viaSearchDepth();
    if (gx_objs) {
        gx_objs->sort();
        Vcheck v(ld1, ld2, gx_objs->objlist(), ol2, stk21);
        if (!v.ol11)
            return (XIok);

        cTfmStack stk;
        for (CDol *o2 = v.ol22; o2; o2 = o2->next) {
            CDo *od2 = o2->odesc;
            BBox BB2(od2->oBB());
            if (stk21)
                stk21->TBB(&BB2, 0);

            CDo *od2t = 0;
            for (CDol *o1 = v.ol11; o1; o1 = o1->next) {
                CDo *od1 = o1->odesc;
                if (od1->oBB().bottom >= BB2.top)
                    continue;
                if (od1->oBB().top <= BB2.bottom)
                    break;
                if (od1->oBB().right <= BB2.left ||
                        od1->oBB().left >= BB2.right)
                    continue;

                if (!od2t)
                    od2t = od2->copyObjectWithXform(stk21);
                if (!od1->intersect(od2t, false))
                    continue;

                BBox xBB(od1->oBB(), BB2);
                xBB.bloat(-1);
                if (xBB.right <= xBB.left || xBB.top <= xBB.bottom)
                    continue;

                this->TBB(&xBB, 0);
                CDo *od1cpy = od1->copyObjectWithXform(this);
                CDo *od2cpy = od2t->copyObjectWithXform(this);
                if (via_depth == 0) {
                    CDg gdesc;
                    gdesc.init_gen(gx_topcell, ldv, &xBB);
                    CDo *odesc;
                    while ((odesc = gdesc.next()) != 0) {
                        XIrt ret = cExt::isConnection(gx_topcell, via, odesc,
                            od1cpy, od2cpy, istrue);
                        if (ret != XIok || *istrue) {
                            delete od1cpy;
                            delete od2cpy;
                            delete od2t;
                            return (ret);
                        }
                    }
                }
                else {
                    sPF gen(gx_topcell, &xBB, ldv, via_depth);
                    CDo *odesc;
                    while ((odesc = gen.next(false, false)) != 0) {
                        XIrt ret = cExt::isConnection(gx_topcell, via, odesc,
                            od1cpy, od2cpy, istrue);
                        delete odesc;
                        if (ret != XIok || *istrue) {
                            delete od1cpy;
                            delete od2cpy;
                            delete od2t;
                            return (ret);
                        }
                    }
                }
                delete od1cpy;
                delete od2cpy;
            }
            delete od2t;
        }
    }
    else if (sdesc()) {
        for (CDol *o2 = ol2; o2; o2 = o2->next) {
            CDo *od2 = o2->odesc;
            if (od2->ldesc() != ld2)
                continue;
            BBox BB2(od2->oBB());
            if (stk21)
                stk21->TBB(&BB2, 0);

            CDo *od2t = 0;
            CDg gd1;
            gd1.init_gen(gx_sdesc, ld1, &BB2);
            CDo *od1;
            while ((od1 = gd1.next()) != 0) {
                if (!od2t)
                    od2t = od2->copyObjectWithXform(stk21);
                if (!od1->intersect(od2t, false))
                    continue;
                BBox xBB(od1->oBB(), BB2);
                xBB.bloat(-1);
                if (xBB.width() <= 0 || xBB.height() <= 0)
                    continue;

                this->TBB(&xBB, 0);
                CDo *od1cpy = od1->copyObjectWithXform(this);
                CDo *od2cpy = od2t->copyObjectWithXform(this);
                if (via_depth == 0) {
                    CDg gdesc;
                    gdesc.init_gen(gx_topcell, ldv, &xBB);
                    CDo *odesc;
                    while ((odesc = gdesc.next()) != 0) {
                        XIrt ret = cExt::isConnection(gx_topcell, via, odesc,
                            od1cpy, od2cpy, istrue);
                        if (ret != XIok || *istrue) {
                            delete od1cpy;
                            delete od2cpy;
                            delete od2t;
                            return (ret);
                        }
                    }
                }
                else {
                    sPF gen(gx_topcell, &xBB, ldv, via_depth);
                    CDo *odesc;
                    while ((odesc = gen.next(false, false)) != 0) {
                        XIrt ret = cExt::isConnection(gx_topcell, via, odesc,
                            od1cpy, od2cpy, istrue);
                        delete odesc;
                        if (ret != XIok || *istrue) {
                            delete od1cpy;
                            delete od2cpy;
                            delete od2t;
                            return (ret);
                        }
                    }
                }
                delete od1cpy;
                delete od2cpy;
            }
            delete od2t;
        }
    }
    return (XIok);
}
// End of sGroupXf functions


// Static function.
// Via connection test.
//
XIrt
cExt::isConnection(CDs *sdesc, const sVia *via, const CDo *odv,
    const CDo *od1, const CDo *od2, bool *connected)
{
    *connected = false;
    Zlist *zv = 0;
    if (odv->type() != CDBOX && ext_via_convex) {
        // Non-box via, probably approximately circular for Hypres. 
        // Test a small box instead for efficiency.

        BBox BB(odv->oBB());
        int d = BB.width()/4;
        BB.left += d;
        BB.right -= d;
        d = BB.height()/4;
        BB.bottom += d;
        BB.top -= d;
        zv = new Zlist(&BB);
    }
    if (!zv)
        zv = odv->toZlist();

    if (via->tree()) {
        try {
            Zlist *zl = zv;
            zv = 0;
            SIlexprCx cx(sdesc, CDMAXCALLDEPTH, zl);

            siVariable v;
            cx.enableExceptions(true);
            if ((*via->tree()->evfunc)(via->tree(), &v, &cx) != OK) {
                Zlist::destroy(zv);
                return (XIbad);
            }
            cx.enableExceptions(false);
            Zlist::destroy(zl);

            if (v.type == TYP_ZLIST) {
                zv = v.content.zlist;
                v.content.zlist = 0;
                zv = Zlist::filter_slivers(zv, 1);
                if (!zv)
                    return (XIok);
            }
        }
        catch (XIrt ret) {
            return (XIintr);
        }
    }
    Zlist *z1 = od1->toZlist();

    XIrt ret = Zlist::zl_and(&zv, z1);
    if (ret != XIok)
        return (ret);
    if (!zv)
        return (XIok);
    Zlist *z2 = od2->toZlist();
    *connected = Zlist::zl_intersect(zv, z2, false);
    return (XIok);
}

