
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
#include "drc.h"
#include "drc_kwords.h"
#include "dsp_inlines.h"
#include "geo_ylist.h"
#include "geo_zgroup.h"
#include "geo_polyobj.h"
#include "si_lexpr.h"
#include "tech.h"
#include "tech_layer.h"
#include "cd_lgen.h"
#include "errorlog.h"


//
// Intrinsic rule evaluation functions.
//


// Initialize the DRC rules, purge any with errors.
//
void
cDRC::initRules()
{
    CDl *ld;
    CDlgenDrv gen;
    while ((ld = gen.next()) != 0) {
        DRCtestDesc *tdn;
        for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = tdn) {
            tdn = td->next();
            if (!td->initialize()) {
                // Errors logged in initialize().
                unlinkRule(td);
                delete td;
            }
        }
    }
}


// Export to delete a rule list.
//
void
cDRC::deleteRules(DRCtestDesc **rp)
{
    if (rp) {
        DRCtestDesc *td = *rp;
        while (td) {
            DRCtestDesc *tdx = td;
            td = td->next();
            delete tdx;
        }
        *rp = 0;
    }
}
// End of cDRC functions.


// A special constructor, mostly for exported utilities.
//
DRCtestDesc::DRCtestDesc(const char *source_exp, const char *in_exp,
    const char *out_exp, const char *target_exp, int dim,
    DRCedgeMode inmode, DRCedgeMode outmode, bool *ok)
{
    *ok = true;
    initTestDesc();
    if (source_exp && *source_exp) {
        const char *s = source_exp;
        if (!td_source.parseExpr(&s, true)) {
            Errs()->add_error("source expression parse failed");
            *ok = false;
            return;
        }
    }
    if (in_exp && *in_exp) {
        const char *s = in_exp;
        if (!td_inside.parseExpr(&s, true)) {
            Errs()->add_error("inside expression parse failed");
            *ok = false;
            return;
        }
    }
    if (out_exp && *out_exp) {
        const char *s = out_exp;
        if (!td_outside.parseExpr(&s, true)) {
            Errs()->add_error("outside expression parse failed");
            *ok = false;
            return;
        }
    }
    if (target_exp && *target_exp) {
        const char *s = target_exp;
        if (!td_target.parseExpr(&s, true)) {
            Errs()->add_error("target expression parse failed");
            *ok = false;
            return;
        }
    }
    td_where = dim > 0 ? DRCoutside : DRCinside;
    td_edge_inside = inmode;
    td_edge_outside = outmode;
    td_u.dimen = abs(dim);
    td_testdim = abs(dim);
    td_testfunc = DRCtestDesc::getZlist;
    td_no_cot = true;
    td_no_skip = true;
}


DRCtestDesc::~DRCtestDesc()
{
    while (td_user_rule) {
        DRCtest *urn = td_user_rule->next();
        delete td_user_rule;
        td_user_rule = urn;
    }
    delete [] td_info[0];
    delete [] td_info[1];
    delete [] td_info[2];
    delete [] td_info[3];
    delete [] td_origstring;

    delete [] td_spacing;
}


// Perform some initialization.
//
bool
DRCtestDesc::initialize()
{
    if (td_initialized)
        return (true);

    // The td_objlayer is a pointer back to the layer containing
    // the rule, the sLspec source is the actual source spec,
    // which can be different.

    if (!td_source.ldesc() && !td_source.tree() && !td_source.lname())
        td_source.set_ldesc(td_objlayer);

    if (!td_source.setup()) {
        const char *ee = Errs()->has_error() ? Errs()->get_error() :
            "unknown error";
        Log()->ErrorLogV(mh::DRC,
           "%s rule source expr on layer %s: %s.", ruleName(),
           td_objlayer->name(), ee);
        return (false);
    }

    switch (td_type) {
    case drNoRule:
        // can't happen
        return (false);
    case drConnected:
    case drNoHoles:
        td_testpoly = &DRCtestDesc::testPoly_NOOP;
        break;
    case drExist:
        td_testpoly = &DRCtestDesc::testPoly_FAIL;
        break;
    case drOverlap:
        td_testpoly = &DRCtestDesc::testPoly_Overlap;
        break;
    case drIfOverlap:
        td_testpoly = &DRCtestDesc::testPoly_IfOverlap;
        break;
    case drNoOverlap:
        td_testpoly = &DRCtestDesc::testPoly_NoOverlap;
        break;
    case drAnyOverlap:
        td_testpoly = &DRCtestDesc::testPoly_AnyOverlap;
        break;
    case drPartOverlap:
        td_testpoly = &DRCtestDesc::testPoly_PartOverlap;
        break;
    case drAnyNoOverlap:
        td_testpoly = &DRCtestDesc::testPoly_AnyNoOverlap;
        break;
    case drMinArea:
        td_testpoly = &DRCtestDesc::testPoly_MinArea;
        break;
    case drMaxArea:
        td_testpoly = &DRCtestDesc::testPoly_MaxArea;
        break;

    case drMinEdgeLength:
    case drMaxWidth:
    case drMinWidth:
    case drMinSpace:
    case drMinSpaceTo:
    case drMinSpaceFrom:
    case drMinOverlap:
    case drMinNoOverlap:
    case drUserDefinedRule:
        td_testpoly = &DRCtestDesc::testPoly_EdgeTest;
        if (!td_inside.setup()) {
            const char *ee = Errs()->has_error() ? Errs()->get_error() :
                "unknown error";
            Log()->ErrorLogV(mh::DRC,
               "%s rule Inside expr on layer %s: %s.", ruleName(),
               td_objlayer->name(), ee);
            return (false);
        }
        if (!td_outside.setup()) {
            const char *ee = Errs()->has_error() ? Errs()->get_error() :
                "unknown error";
            Log()->ErrorLogV(mh::DRC,
               "%s rule Outside expr on layer %s: %s.", ruleName(),
               td_objlayer->name(), ee);
            return (false);
        }
        break;
    }

    switch (td_type) {
    case drNoRule:
        // can't happen
        return (false);
    case drConnected:
    case drNoHoles:
    case drExist:
    case drMinSpace:
    case drMinArea:
    case drMaxArea:
    case drMaxWidth:
    case drMinWidth:
        break;
    case drMinEdgeLength:
    case drMinSpaceTo:
    case drMinSpaceFrom:
    case drOverlap:
    case drIfOverlap:
    case drNoOverlap:
    case drAnyOverlap:
    case drPartOverlap:
    case drAnyNoOverlap:
    case drMinOverlap:
    case drMinNoOverlap:
        if (!td_target.setup()) {
            const char *ee = Errs()->has_error() ? Errs()->get_error() :
                "unknown error";
            Log()->ErrorLogV(mh::DRC,
               "%s rule target expr on layer %s: %s.", ruleName(),
               td_objlayer->name(), ee);
            return (false);
        }
        break;
    case drUserDefinedRule:
        char *ee = td_user_rule->init();
        if (ee) {
            Log()->ErrorLogV(mh::DRC,
                "%s rule on layer %s: %s.",
                td_user_rule->name(), td_objlayer->name(), ee);
            delete ee;
            return (false);
        }
        break;
    }

    initEdgeTest();
    td_initialized = true;
    return (true);
}


// Return the text of the rule's name.
//
const char *
DRCtestDesc::ruleName() const
{
    if (td_type == drUserDefinedRule && td_user_rule)
        return (td_user_rule->name());
    return (ruleName((DRCtype)td_type));
}


// Return the Region string, if any.  Null is returned if there is
// no string needed, or if the region spec is bad.
//
char *
DRCtestDesc::regionString() const
{
    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        // td_source.ldesc() must be set to td_objlayer if there is no
        // Region spec.

        if (!td_source.tree()) {
            if (td_source.ldesc())
                return (lstring::copy(td_source.ldesc()->name()));
            if (td_source.lname())
                return (lstring::copy(td_source.lname()));
            return (0);
        }

        // The tree is the region ANDed with the current layer.
        ParseNode *p = td_source.tree();
        if (p->optype != TOK_AND || !p->left || !p->right) {
            // Huh? No good.
            return (0);
        }
        if (p->left->type == PT_VAR) {
            if (!p->left->data.v->name)
                return (0);
            if (!strcmp(p->left->data.v->name, td_objlayer->name())) {
                sLstr lstr;
                p->right->string(lstr);
                return (lstr.string_trim());
            }
        }
        else if (p->right->type == PT_VAR) {
            if (!p->right->data.v->name)
                return (0);
            if (!strcmp(p->right->data.v->name, td_objlayer->name())) {
                sLstr lstr;
                p->left->string(lstr);
                return (lstr.string_trim());
            }
        }
    }
    return (0);
}


//
//  ===========  RULE EVALUATION  ================================
//

// NOTE:  the cDRC::fudgeVal() is used to filter slivers and otherwise
// avoid false positives in non-Manhattan processing.  Recommended
// values are:
//   fully Manhattan    0
//   45s only           2
//   all angles         4
// The value is mirrored in DRCtestDesc::td_fudge.
//

// Summary of internal rule setup/test.
//  src_edge_in x
//  src_edge_out u
//                      where  test   t_edge_in  t_edge_out
//  drMinEdgeLength     in     len    c          c
//  drMaxWidth          in     nf
//  drMinWidth          in     f
//  drMinSpace          out    e
//  drMinSpaceTo        out    e      u          x
//  drMinSpaceFrom      out    f      c          x
//  drMinOverlap        in     f      c          x
//  drMinNoOverlap      in     e      u          x


// Initialize for the edge tests.
//
void
DRCtestDesc::initEdgeTest()
{
    // Set the fudge value, eventually we probably want a separate
    // setting from the ui.
    DRC()->setFudgeVal(2*Tech()->AngleSupport());
    td_fudge = DRC()->fudgeVal();

    td_ang_min = ANG_MIN;
    td_has_target = requiresTarget((DRCtype)td_type);
    if (td_type == drMinEdgeLength) {
        td_where = DRCinside;
        td_edge_outside = EdgeCovered;
        td_edge_inside = EdgeCovered;
        td_no_test_corners = true;
        td_no_area = true;
        td_testfunc = &testEdgeLen;
    }
    else if (td_type == drMaxWidth) {
        td_where = DRCinside;
        td_testfunc = &testNotFull;
    }
    else if (td_type == drMinWidth) {
        td_where = DRCinside;
        td_testfunc = &testFull;
    }
    else if (td_type == drMinSpace) {
        td_where = DRCoutside;
        td_testfunc = &testEmpty;
    }
    else if (td_type == drMinSpaceTo) {
        // MinSpaceTo: inside edge clear => test outside clear
        td_where = DRCoutside;
        td_edge_inside = EdgeClear;
        td_testfunc = &testEmpty;
    }
    else if (td_type == drMinSpaceFrom) {
        // MinSpaceFrom: inside edge covered => test outside covered
        td_where = DRCoutside;
        td_edge_inside = EdgeCovered;
        td_testfunc = &testFull;
        if ((hasValue(1) || hasValue(2)) && value(1) != value(2))
            td_opptest = true;

        // We may have Enclosed and Opposite clauses.  Enclosed is the
        // same as Opposite with both values the same.  If either is
        // given
        // 1.  The figure must be covered by the target.
        // 2.  The figure must be rectangular.
        // 3.  If Enclosed, check corners too.
        // 4.  If Opposite, check sides only unless the two values are equal.
        // 5.  If a dimension is 0, always pass.
    }
    else if (td_type == drMinOverlap) {
        // MinOverlap: inside edge covered => test inside covered
        td_where = DRCinside;
        td_edge_inside = EdgeCovered;
        td_testfunc = &testFull;
    }
    else if (td_type == drMinNoOverlap) {
        // MinNoOverlap: inside edge clear => test inside clear
        td_where = DRCinside;
        td_edge_inside = EdgeClear;
        td_testfunc = &testEmpty;
    }
}


XIrt
DRCtestDesc::initEdgeList(DRCedgeEval *ev)
{
    if (!ev)
        return (XIbad);
    if (!ev->last_vtx()) {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return (XIbad);
        int ecode;
        Zlist *elist;
        // Edge segments that are not covered by source.
        XIrt ret = cGEO::edge_zlist(cursdp, &td_source, &ev->p2(), &ev->p3(),
            ev->outside(), false, &ecode, &elist);
        if (ret == XIok) {
            ev->set_edgecode(ecode);
            ev->set_elist(elist);
        }
        else
            return (ret);
    }
    ev->initAng();
    return (XIok);
}


// Compute the list of segments of the edge where tests are applied.
//
XIrt
DRCtestDesc::getEdgeList(DRCedgeEval *ev, Zlist **zret, bool *copy)
{
    *zret = 0;
    *copy = false;
    if (!ev->elist())
        return (XIok);
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (XIbad);
    bool cpy = false;
    Zlist *z0 = ev->elist();

    if (td_user_rule && td_user_rule->edges()) {
        for (DRCedgeCnd *e = td_user_rule->edges(); e; e = e->next()) {
            Zlist *zl;
            XIrt ret =
                cGEO::edge_zlist(cursdp, e->lspec(), &ev->p2(), &ev->p3(),
                e->where() == DRCoutside ? ev->outside() : ev->inside(),
                true, 0, &zl);
            if (ret != XIok) {
                if (cpy)
                    Zlist::destroy(z0);
                return (ret);
            }
            Zlist *zn = cGEO::merge_edge_zlists(z0, ev->edgecode(), zl);
            if (cpy)
                Zlist::destroy(z0);
            if (!zn)
                return (XIok);
            z0 = zn;
            cpy = true;
        }
    }
    else if (hasTarget()) {
        if (edgeInside() != EdgeDontCare) {
            Zlist *zl;
            XIrt ret =
                cGEO::edge_zlist(cursdp, &td_target, &ev->p2(), &ev->p3(),
                ev->inside(), (edgeInside() == EdgeCovered), 0, &zl);
            if (ret != XIok) {
                if (cpy)
                    Zlist::destroy(z0);
                return (ret);
            }
            Zlist *zn = cGEO::merge_edge_zlists(z0, ev->edgecode(), zl);
            if (cpy)
                Zlist::destroy(z0);
            if (!zn)
                return (XIok);
            z0 = zn;
            cpy = true;
        }
        if (edgeOutside() != EdgeDontCare) {
            Zlist *zl;
            XIrt ret =
                cGEO::edge_zlist(cursdp, &td_target, &ev->p2(), &ev->p3(),
                ev->outside(), (edgeOutside() == EdgeCovered), 0, &zl);
            if (ret != XIok) {
                if (cpy)
                    Zlist::destroy(z0);
                return (ret);
            }
            Zlist *zn = cGEO::merge_edge_zlists(z0, ev->edgecode(), zl);
            if (cpy)
                Zlist::destroy(z0);
            if (!zn)
                return (XIok);
            z0 = zn;
            cpy = true;
        }
    }
    if (hasInside()) {
        Zlist *zl;
        XIrt ret = cGEO::edge_zlist(cursdp, &td_inside,
            &ev->p2(), &ev->p3(), ev->inside(), true, 0, &zl);
        if (ret != XIok) {
            if (cpy)
                Zlist::destroy(z0);
            return (ret);
        }
        Zlist *zn = cGEO::merge_edge_zlists(z0, ev->edgecode(), zl);
        if (cpy)
            Zlist::destroy(z0);
        if (!zn)
            return (XIok);
        z0 = zn;
        cpy = true;
    }
    if (hasOutside()) {
        Zlist *zl;
        XIrt ret = cGEO::edge_zlist(cursdp, &td_outside,
            &ev->p2(), &ev->p3(), ev->outside(), true, 0, &zl);
        if (ret != XIok) {
            if (cpy)
                Zlist::destroy(z0);
            return (ret);
        }
        Zlist *zn = cGEO::merge_edge_zlists(z0, ev->edgecode(), zl);
        if (cpy)
            Zlist::destroy(z0);
        if (!zn)
            return (XIok);
        z0 = zn;
        cpy = true;
    }
    *zret = z0;
    *copy = cpy;
    return (XIok);
}


// Do any rule pre-tests that are performed before the edge tests.
//
XIrt
DRCtestDesc::polyEdgesPreTest(DRCedgeEval *ev, ErrFuncCb err_cb, void *arg)
{
    if (td_user_rule)
        return (XIok);

    setResult(0);
    if (type() == drMinSpaceFrom) {
        if (hasValue(0) || hasValue(1) || hasValue(2)) {
            // Enclosed or Opposite  specified, test that the poly is
            // completely covered.  Return IFOVL if partial covering,
            // this return triggers a modified error message.

            SIlexprCx cx(0, CDMAXCALLDEPTH, ev->epoly()->zlist());
            CovType cov;
            XIrt ret = td_target.testZlistCovPartial(&cx, &cov, td_fudge);
            if (ret != XIok)
                return (ret);
            if (cov == CovPartial) {
                BBox BB;
                Zlist::BB(cx.getZref(), BB);
                Zoid Z(&BB);
                seterr(&Z, TT_IFOVL);
                if (result() && err_cb)
                    (*err_cb)(this, ev->edge_indx(), arg);
            }
        }
    }
    return (XIok);
}


// Evaluate the test over the the current edge and corner.  If err_cb
// is given, this will be called when a DRC error is indicated.  The
// function returns if err_cb returns false, otherwise it will continue.
// If err_cb is not given, the function returns on the first error.
//
XIrt
DRCtestDesc::polyEdgesTest(DRCedgeEval *ev, ErrFuncCb err_cb, void *arg)
{
    Zlist *elist = 0;
    bool ecopy = false;
    if (!ev->last_vtx()) {
        // Find the list of segments of the edge to test
        XIrt ret = getEdgeList(ev, &elist, &ecopy);
        if (ret != XIok)
            return (ret);

        // Set the corner segment widths
        int cw1, cw2;
        cGEO::edge_overlap(elist, ev->edgecode(), &ev->p2(), &ev->p3(),
            &cw1, &cw2);
        initCw(cw1, cw2, (ev->passcnt() == 1));
    }
    else
        finalCw();

    setResult(0);
    edgeSetDimen(ev);
    if (ev->passcnt() != 1 && !noTestCorners() &&
            !(oppTest() && ev->epoly()->isrect())) {
        // Do the corner tests

        if (testCw() &&
                ((where() == DRCoutside && doCST(ev->ang())) ||
                (where() == DRCinside && doCWT(ev->ang())))) {

            // If we're using a spacing table, use the default
            // dimension for the corner tests.
            int dim = 0;
            if (spaceTab()) {
                dim = testDim();
                setTestDim(spaceTab()->dimen);
            }
            XIrt ret = td_user_rule ?
                td_user_rule->cornerTest(this, ev) : cornerTest(ev);
            if (spaceTab())
                setTestDim(dim);

            if (ret != XIok) {
                if (ecopy)
                    Zlist::destroy(elist);
                return (ret);
            }
            if (result()) {
                if (!err_cb)
                    goto finish;
                if (!(*err_cb)(this, ev->corner_indx(), arg))
                    goto finish;
                setResult(0);
            }
        }
        if (!result() && doCOT(ev->ang()) && where() == DRCinside &&
                ((td_user_rule && td_user_rule->cotWidth() > 0) ||
                (!td_user_rule && type() == drMinWidth))) {

            XIrt ret = td_user_rule ?
                td_user_rule->cornerOverlapTest(this, ev) :
                cornerOverlapTest(ev, testDim());

            if (ret != XIok) {
                if (ecopy)
                    Zlist::destroy(elist);
                return (ret);
            }
            if (result()) {
                if (!err_cb)
                    goto finish;
                if (!(*err_cb)(this, ev->corner_indx(), arg))
                    goto finish;
                setResult(0);
            }
        }
    }

    if (!ev->last_vtx() && elist) {

        // Can we skip edge test?
        bool dochk = true;
        if (td_user_rule)
            dochk =
                (!ev->epoly()->isrect() || !td_user_rule->cotStr() ||
                ev->passcnt() <= 2);
        else if (!hasTarget() && where() == DRCinside && !noSkip()) {
            bool iswire = ev->wireCheck(testDim());
            dochk = (!iswire || ev->check_wire_edge()) &&
                (!ev->epoly()->isrect() || ev->passcnt() <= 2);
        }

        // Do the edge test if not skipping
        if (dochk) {

            XIrt ret = td_user_rule ?
                td_user_rule->edgeTest(this, ev, elist) :
                edgeTest(ev, elist);

            if (ret != XIok) {
                if (ecopy)
                    Zlist::destroy(elist);
                return (ret);
            }
            if (result() && err_cb)
                (*err_cb)(this, ev->edge_indx(), arg);
        }
    }

finish:
    if (ecopy)
        Zlist::destroy(elist);
    lastCw();
    return (XIok);
}


namespace {
    inline double
    distance(const Point *p1, const Point *p2)
    {
        return (sqrt((p1->x - p2->x)*(double)(p1->x - p2->x) +
            (p1->y - p2->y)*(double)(p1->y - p2->y)));
    }
}


// Set the td_testdim to the apporpriate dimension for the test.  This
// must be set before constructing the test area.
//
void
DRCtestDesc::edgeSetDimen(const DRCedgeEval *ev)
{
    bool manh = false;
    switch (ev->edgecode()) {
    case DRCeB:
    case DRCeT:
    case DRCeL:
    case DRCeR:
        manh = true;
    default:
        break;
    }

    td_testdim = dimen();
    td_testdim2 = 0;

    // For MaxWidth, we look at inside-coverage of edges longer than
    // td_dimen.  If fully covered for td_dimen + 1, an error is
    // assumed.  The unit-count extension is fine in the Manhattan
    // case, but a larger value is required for non-Manhattan to
    // overcome the "fudge factor".

    if (manh) {
        if (td_type == drMaxWidth)
            td_testdim += 1;

        else if (td_type == drMinSpaceFrom) {

            // Handle Enclosed/Opposite
            if (hasValue(0))
                td_testdim = value(0);
            else if ((hasValue(1) || hasValue(2))) {
                if (value(1) == value(2))
                    td_testdim = value(1);
                else if (value(1) > value(2)) {
                    td_testdim = value(1);
                    td_testdim2 = value(2);
                }
                else {
                    td_testdim = value(2);
                    td_testdim2 = value(1);
                }
            }
        }
        else if (td_spacing && !td_spacing->length) {
            // Use the spacing table if any.  For now, only
            // width-length tables are used, and only on Manhattan
            // edges.

            int width = ev->epoly()->eff_width();
            int dx = ev->p2().x - ev->p3().x;
            int dy = ev->p2().y - ev->p3().y;
            int length;
            if (dy == 0)
                length = abs(dx);
            else if (dx == 0)
                length = abs(dy);
            else
                length = mmRnd(sqrt(dx*(double)dx + dy*(double)dy));
            td_testdim = td_spacing->evaluate(width, length);
        }
    }
    else {
        if (td_type == drMaxWidth) {
            td_testdim += 2*td_fudge + 1;
        }
        else if (td_type == drMinWidth) {
            // Use Diagonal value if given.
            if (td_values[0] > 0)
                td_testdim = td_values[0];
        }
        else if (td_type == drMinSpace) {
            // Use Diagonal value if given.
            if (td_values[0] > 0)
                td_testdim = td_values[0];
        }
        else if (td_type == drMinSpaceTo) {
            // Use Diagonal value if given.
            if (td_values[0] > 0)
                td_testdim = td_values[0];
        }
        td_testdim -= td_fudge;
    }
}


// Construct the test area.  Return false and the test zoid in zret, or
// true with the test poly in pret[5].
//
bool
DRCtestDesc::edgeTestArea(const Point &p2, const Point &p3, const Zoid *edge,
    int code, Zoid *zret, Point *pret)
{
    if (code == DRCeB) {
        *zret = *edge;
        if (td_where == DRCinside) {
            zret->yl = p2.y - td_testdim;
            zret->yu = p2.y;
        }
        else {
            zret->yu = p2.y + td_testdim;
            zret->yl = p2.y;
        }
        return (false);
    }
    if (code == DRCeT) {
        *zret = *edge;
        if (td_where == DRCinside) {
            zret->yl = p2.y;
            zret->yu = p2.y + td_testdim;
        }
        else {
            zret->yl = p2.y - td_testdim;
            zret->yu = p2.y;
        }
        return (false);
    }
    if (code == DRCeL) {
        *zret = *edge;
        if (td_where == DRCinside) {
            zret->xul = zret->xll = p2.x - td_testdim;
            zret->xur = zret->xlr = p2.x;
        }
        else {
            zret->xur = zret->xlr = p2.x + td_testdim;
            zret->xul = zret->xll = p2.x;
        }
        return (false);
    }
    if (code == DRCeR) {
        *zret = *edge;
        if (td_where == DRCinside) {
            zret->xul = zret->xll = p2.x;
            zret->xur = zret->xlr = p2.x + td_testdim;
        }
        else {
            zret->xul = zret->xll = p2.x - td_testdim;
            zret->xur = zret->xlr = p2.x;
        }
        return (false);
    }

    // Non-orthogonal case
    // The fudge factor is used to slightly shrink and offset the created
    // poly, to avoid false results in DRC coverage tests due to numerical
    // errors

    double d = distance(&p2, &p3);
    int dx = p3.x - p2.x;
    int dy = p3.y - p2.y;
    double co, si;
    if ((td_where == DRCinside &&
            (code == DRCeCWA || code == DRCeCWD)) ||
            (td_where == DRCoutside &&
            (code == DRCeCCWA || code == DRCeCCWD))) {
        co = dx/d;
        si = dy/d;
    }
    else {
        co = -dx/d;
        si = -dy/d;
    }
    if (td_fudge) {
        double fx = 0.5*co*td_fudge;
        double fy = 0.5*si*td_fudge;
        if ((td_where == DRCinside &&
                (code == DRCeCWA || code == DRCeCCWD)) ||
                (td_where == DRCoutside &&
                (code == DRCeCCWA || code == DRCeCWD))) {

            pret[0].set(mmRnd(edge->xll + fy + fx),
                mmRnd(edge->yl - fx + fy));
            pret[1].set(mmRnd(edge->xur + fy - fx),
                mmRnd(edge->yu - fx - fy));
        }
        else {
            pret[0].set(mmRnd(edge->xll + fy - fx),
                mmRnd(edge->yl - fx - fy));
            pret[1].set(mmRnd(edge->xur + fy + fx),
                mmRnd(edge->yu - fx + fy));
        }
    }
    else {
        pret[0].set(edge->xll, edge->yl);
        pret[1].set(edge->xur, edge->yu);
    }

    double ddx = co*td_testdim;
    double ddy = si*td_testdim;
    pret[2].set(mmRnd(pret[1].x + ddy), mmRnd(pret[1].y - ddx));
    pret[3].set(mmRnd(pret[0].x + ddy), mmRnd(pret[0].y - ddx));
    pret[4] = pret[0];
    return (true);
}


namespace {
    int edge_length(const Zoid *Z)
    {
        if (Z->yu == Z->yl)
            return (abs(Z->xur - Z->xll));
        if (Z->xur == Z->xll)
            return (abs(Z->yu - Z->yl));
        return (int)sqrt((Z->yu - Z->yl)*(double)(Z->yu - Z->yl) +
            (Z->xur - Z->xll)*(double)(Z->xur - Z->xll));
    }
}


namespace {
    // Check if the Opp test passes.  The array has results from
    // the four edges, the values can be
    //  0  outright fail
    //  1  pass for larger dimension
    //  2  pass for smaller dimension
    //
    // The return values are
    //  0  outright fail (already recorded)
    //  -1 fail
    //  1  pass
    //
    inline int check_opp(const char *a)
    {
        if (!a[0] || !a[1] || !a[2] || !a[3])
            return (0);
        if (a[0] == 1 && a[2] == 1)
            return (1);
        if (a[1] == 1 && a[3] == 1)
            return (1);
        return (-1);
    }
}


// Evaluate the test for an edge.
//
XIrt
DRCtestDesc::edgeTest(DRCedgeEval *ev, const Zlist *elist)
{
    td_result = 0;
    if (td_testdim <= 0)
        return (XIok);
    for (const Zlist *zl = elist; zl; zl = zl->next) {
        if (td_result)
            break;
        Zoid ZB;
        Point po[5];
        if (td_no_area) {
            ZB = zl->Z;
            bool istrue;
            XIrt ret = (*td_testfunc)(&ZB, this, ev, &istrue);
            if (ret != XIok)
                return (ret);
            if (!istrue)
                seterr(&ZB, TT_ELT);
            continue;
        }
        if (td_type == drMaxWidth) {
            if (edge_length(&zl->Z) <= dimen())
                continue;
        }

        if (!edgeTestArea(ev->p2(), ev->p3(), &zl->Z, ev->edgecode(),
                &ZB, po)) {

            if ((td_type == drMinSpace || td_type == drMinSpaceTo) &&
                    td_spacing && !td_spacing->length) {
                // Using space table.  Initially, the dimension is
                // set based on the total edge length.  If this
                // fails, we measure the discovered overlap length,
                // and reevaluate the dimension from the table.  If
                // the dimension becomes smaller, test again.  If
                // not, an error is indicated.

                int width = ev->epoly()->eff_width();
                int length = 0;
                for (;;) {
                    XIrt ret = spaceTest(&ZB, ev, &length);
                    if (ret != XIok)
                        return (ret);
                    if (!length)
                        break;
                    int dim = td_spacing->evaluate(width, length);
                    if (dim < td_testdim) {
                        td_testdim = dim;
                        edgeTestArea(ev->p2(), ev->p3(), &zl->Z,
                            ev->edgecode(), &ZB, po);
                        continue;
                    }
                    break;
                }
                if (length)
                    seterr(&ZB, td_where == DRCoutside ? TT_EST : TT_EWT);
                if (DRC()->isShowZoids() && DSP()->MainWdesc())
                    ZB.show();
                if (td_testfunc == DRCtestDesc::getZlist) {
                    // Hack here, we're supposed to be drawing test
                    // regions only, which is the purpose of this
                    // testfunc, so we need to call it.

                    bool istrue;
                    XIrt ret = (*td_testfunc)(&ZB, this, ev, &istrue);
                    if (ret != XIok)
                        return (ret);
                }
                continue;
            }

            if (DRC()->isShowZoids() && DSP()->MainWdesc())
                ZB.show();
            int rslt = 0;
            bool istrue;
            XIrt ret = (*td_testfunc)(&ZB, this, ev, &istrue);
            if (ret != XIok)
                return (ret);
            if (istrue)
                rslt = 1;
            else if (td_opptest && ev->epoly()->isrect()) {
                // This is for the Opp test.  If we fail on the larger
                // dimension, try again with the smaller.  We'll defer
                // making the final pass/fail judgement until the
                // final edge, unless we get an outright fail earlier.

                istrue = true;
                int tmp = td_testdim;
                td_testdim = td_testdim2;
                if (td_testdim > 0) {
                    edgeTestArea(ev->p2(), ev->p3(),
                        &zl->Z, ev->edgecode(), &ZB, po);
                    if (DRC()->isShowZoids() && DSP()->MainWdesc())
                        ZB.show();
                    ret = (*td_testfunc)(&ZB, this, ev, &istrue);
                    if (ret != XIok)
                        return (ret);
                    if (istrue)
                        rslt = 2;
                }
                else
                    rslt = 2;
                td_testdim = tmp;
            }
            if (!rslt)
                seterr(&ZB, td_where == DRCoutside ? TT_EST : TT_EWT);
            if (ev->edge_indx() < 4) {
                td_edgrslt[ev->edge_indx()] = rslt;
                if (ev->edge_indx() == 3) {
                    if (check_opp(td_edgrslt) < 0) {
                        const Zlist *zz = ev->epoly()->zlist();
                        seterr(&zz->Z, TT_OPP);
                    }
                }
            }
        }
        else {
            Zlist *zx0 = Point::toZlist(po, 5);
            if (zx0) {
                if (DRC()->isShowZoids() && DSP()->MainWdesc())
                    Zlist::show(zx0);
                for (Zlist *zx = zx0; zx; zx = zx->next) {
                    bool istrue;
                    XIrt ret = (*td_testfunc)(&zx->Z, this, ev, &istrue);
                    if (ret != XIok) {
                        Zlist::destroy(zx0);
                        return (ret);
                    }
                    if (!istrue) {
                        seterr(po, td_where == DRCoutside ? TT_EST : TT_EWT);
                        break;
                    }
                }
                Zlist::destroy(zx0);
            }
        }
    }
    return (XIok);
}


// Return the polygon defining the region for a corner test in pret.  False
// is returned if the polygon has no area.
//
bool
DRCtestDesc::cornerTestArea(const Point &p1, const Point &p2, const Point &p3,
    bool cw, Point *pret)
{
    int width = (td_where == DRCoutside ? -td_testdim : td_testdim);
    Point pp;
    if (p1.x == p2.x)
        pp.set(p2.x + ((cw && p2.y > p1.y) || (!cw && p2.y < p1.y) ?
            width : -width), p2.y);
    else if (p1.y == p2.y)
        pp.set(p2.x, p2.y + ((cw && p2.x > p1.x) || (!cw && p2.x < p1.x) ?
            -width : width));
    else {
        double d1 = distance(&p1, &p2);
        d1 = width/d1;
        if (cw)
            pp.set(mmRnd(p2.x - (p1.y - p2.y)*d1),
                mmRnd(p2.y + (p1.x - p2.x)*d1));
        else
            pp.set(mmRnd(p2.x + (p1.y - p2.y)*d1),
                mmRnd(p2.y - (p1.x - p2.x)*d1));
    }
    Point pq;
    if (p3.x == p2.x)
        pq.set(p2.x + ((cw && p2.y < p3.y) || (!cw && p2.y > p3.y) ?
            width : -width), p2.y);
    else if (p3.y == p2.y)
        pq.set(p2.x, p2.y + ((cw && p2.x < p3.x) || (!cw && p2.x > p3.x) ?
            -width : width));
    else {
        double d2 = distance(&p2, &p3);
        d2 = width/d2;
        if (cw)
            pq.set(mmRnd(p2.x + (p3.y - p2.y)*d2),
                mmRnd(p2.y - (p3.x - p2.x)*d2));
        else
            pq.set(mmRnd(p2.x - (p3.y - p2.y)*d2),
                mmRnd(p2.y + (p3.x - p2.x)*d2));
    }

    if (width < 0)
        width = -width;

    // Create a poly with pq, p2, pp, and a point width away from p2,
    // along the bisector of pp and pq.
    //
    pret[0] = pp;
    pret[1] = p2;
    pret[2] = pq;
    int x = (pp.x + pq.x)/2;
    int y = (pp.y + pq.y)/2;
    int dx = x - p2.x;
    int dy = y - p2.y;
    double d = sqrt(dx*(double)dx + dy*(double)dy);
    if (d == 0)
        return (false);
    d = width/d;
    pret[3].set(mmRnd(p2.x + dx*d), mmRnd(p2.y + dy*d));
    pret[4] = pret[0];
    return (true);
}


// Evaluate a corner test.
//
XIrt
DRCtestDesc::cornerTest(DRCedgeEval *ev)
{
    td_result = 0;
    Point po[5];
    if (!cornerTestArea(ev->p1(), ev->p2(), ev->p3(), ev->cw(), po))
        return (XIok);

    Zlist *zx0 = Point::toZlist(po, 5);
    if (DRC()->isShowZoids() && DSP()->MainWdesc())
        Zlist::show(zx0);
    for (Zlist *zx = zx0; zx; zx = zx->next) {
        bool istrue;
        XIrt ret = (*td_testfunc)(&zx->Z, this, ev, &istrue);
        if (ret != XIok) {
            Zlist::destroy(zx0);
            return (ret);
        }
        if (!istrue) {
            seterr(po, td_where == DRCoutside ? TT_CST : TT_CWT);
            break;
        }
    }
    Zlist::destroy(zx0);
    return (XIok);
}


XIrt
DRCtestDesc::cornerOverlapTest(DRCedgeEval *ev, int width)
{
    // Check for sufficient overlap with other objects
    td_result = 0;
    const Point &p1 = ev->p1();
    const Point &p2 = ev->p2();
    const Point &p3 = ev->p3();
    int d12 = td_lcw;
    int d23 = td_cw1;
    if (!d12 && !d23)
        return (XIok);
    if (d12 >= width && d23 >= width)
        return (XIok);

    if (d12 < width && d23 < width) {
        if (td_testfunc == &getZlist)
            return (XIok);
        if (td_no_cot)
            return (XIok);

        // Measure the distance between the two points of overlap, must
        // be >= width
        Point pp, pq;
        if (d12 > 0) {
            if (p1.x == p2.x) {
                pp.set(p2.x, p2.y + (p2.y > p1.y ? -d12 : d12));
                // if entirely covered, we're ok
                if (pp.y == p1.y)
                    return (XIok);
            }
            else if (p1.y == p2.y) {
                pp.set(p2.x + (p2.x > p1.x ? -d12 : d12), p2.y);
                if (pp.x == p1.x)
                    return (XIok);
            }
            else {
                double d1 = distance(&p1, &p2);
                if (fabs(d1 - d12) <= 0.5*td_fudge)
                    return (XIok);
                pp.set(mmRnd(p2.x + d12*((p1.x - p2.x)/d1)),
                    mmRnd(p2.y + d12*((p1.y - p2.y)/d1)));
            }
        }
        else
            pp = p2;
        if (d23 > 0) {
            if (p3.x == p2.x) {
                pq.set(p2.x, p2.y + (p2.y > p3.y ? -d23 : d23));
                if (pq.y == p3.y)
                    return (XIok);
            }
            else if (p3.y == p2.y) {
                pq.set(p2.x + (p2.x > p3.x ? -d23 : d23), p2.y);
                if (pq.x == p3.x)
                    return (XIok);
            }
            else {
                double d2 = distance(&p2, &p3);
                if (fabs(d2 - d23) <= 2)
                    return (XIok);
                pq.set(mmRnd(p2.x + d23*((p3.x - p2.x)/d2)),
                    mmRnd(p2.y + d23*((p3.y - p2.y)/d2)));
            }
        }
        else
            pq = p2;
        if (pp.x == pq.x) {
            pp.y -= pq.y;
            if (pp.y && width > (pp.y > 0 ? pp.y : -pp.y)) {
                seterr_pt(&p2, TT_COT);
                return (XIok);
            }
        }
        else if (pp.y == pq.y) {
            pp.x -= pq.x;
            if (pp.x && width > (pp.x > 0 ? pp.x : -pp.x)) {
                seterr_pt(&p2, TT_COT);
                return (XIok);
            }
        }
        else {
            double dp = (double)(pp.x - pq.x)*(pp.x - pq.x) +
                (double)(pp.y - pq.y)*(pp.y - pq.y);
            if (dp < (double)width*width) {
                seterr_pt(&p2, TT_COT);
                return (XIok);
            }
        }
    }
    else {
        // See if the edge with large overlap continues past the edge,
        // forming an "inside" corner.  If so, do a "corner width" test.
        // This should catch errors such as two corner-corner boxes
        // separated by a thin third box:
        //             |       |
        //             |       |
        //   |=================|
        //   |         |
        //   |         |
        //

        int dx1, dy1;
        int dx2, dy2;
        int dmin;
        if (d12 < width && d23 >= width) {
            dx1 = p2.x - p1.x;
            dy1 = p2.y - p1.y;
            dx2 = p2.x - p3.x;
            dy2 = p2.y - p3.y;
            dmin = d12;
        }
        else if (d12 >= width && d23 < width) {
            dx1 = p2.x - p3.x;
            dy1 = p2.y - p3.y;
            dx2 = p2.x - p1.x;
            dy2 = p2.y - p1.y;
            dmin = d23;
        }
        else
            return (XIok);

        Point pp, xp;
        Point pq, xq;
        Point pz;
        if (dx1 == 0) {
            if (dmin == abs(dy1))
                return (XIok);
            pq.set(p2.x, dy1 > 0 ? p2.y + 10 : p2.y - 10);
            pz.set(p2.x, dy1 > 0 ? p2.y - dmin : p2.y + dmin);
            xq.set(p2.x, dy1 > 0 ? p2.y + width - dmin : p2.y - width + dmin);
        }
        else if (dy1 == 0) {
            if (dmin == abs(dx1))
                return (XIok);
            pq.set(dx1 > 0 ? p2.x + 10 : p2.x - 10, p2.y);
            pz.set(dx1 > 0 ? p2.x - dmin : p2.x + dmin, p2.y);
            xq.set(dx1 > 0 ? p2.x + width - dmin : p2.x - width + dmin, p2.y);
        }
        else {
            double d = sqrt(dx1*(double)dx1 + dy1*(double)dy1);
            if (d-2 <= dmin && dmin <= d+2)
                return (XIok);
            pq.set(mmRnd(p2.x + 10*(dx1/d)), mmRnd(p2.y + 10*(dy1/d)));
            xq.set(mmRnd(p2.x + (width - dmin)*(dx1/d)),
                mmRnd(p2.y + (width - dmin)*(dy1/d)));
            pz.set(mmRnd(p2.x - dmin*(dx1/d)), mmRnd(p2.y - dmin*(dy1/d)));
        }

        if (dx2 == 0) {
            pp.set(p2.x, dy2 > 0 ? p2.y + 10 : p2.y - 10);
            xp.set(pz.x, dy2 > 0 ? pz.y - width : pz.y + width);
        }
        else if (dy2 == 0) {
            pp.set(dx2 > 0 ? p2.x + 10 : p2.x - 10, p2.y);
            xp.set(dx2 > 0 ? pz.x - width : pz.x + width, pz.y);
        }
        else {
            double d = sqrt(dx2*(double)dx2 + dy2*(double)dy2);
            pp.set(mmRnd(p2.x + 10*(dx2/d)), mmRnd(p2.y + 10*(dy2/d)));
            xp.set(mmRnd(pz.x - width*(dx2/d)), mmRnd(pz.y - width*(dy2/d)));
        }

        Point po[5];
        bool istrue = true;
        if (dmin == 0) {
            po[0] = p2;
            po[1] = pp;
            po[2].set(pp.x + (pq.x - p2.x), pp.y + (pq.y - p2.y));
            po[3] = pq;
            po[4] = p2;
            XIrt ret = testPolyCovered(po, ev, &istrue);
            if (ret != XIok)
                return (ret);
        }
        if (istrue) {
            po[0] = xp;
            po[1] = pz;
            po[2] = xq;
            int x = (xp.x + xq.x)/2;
            int y = (xp.y + xq.y)/2;
            int dx = x - pz.x;
            int dy = y - pz.y;
            double d = sqrt(dx*(double)dx + dy*(double)dy);
            if (d == 0)
                return (XIok);
            d = width/d;
            po[3].set(mmRnd(pz.x + dx*d), mmRnd(pz.y + dy*d));
            po[4] = po[0];
            XIrt ret = testPolyCovered(po, ev, &istrue);
            if (ret != XIok)
                return (ret);
            if (!istrue) {
                seterr(po, TT_COT);
                return (XIok);
            }
        }
    }
    return (XIok);
}


//
// Error reporting functions.
//

// Error handling for tree evaluation errors.  For evaluation errors, pop
// up an error message, and inhibit the rule.
//
void
DRCtestDesc::treeError()
{
    sLstr lstr;
    print(0, &lstr, false, 0, false, false);
    Log()->ErrorLogV(mh::DRC,
        "Evaluation error for rule on layer %s:\n%sRule inhibited.",
        td_objlayer->name(), lstr.string());
    td_inhibit |= 1;
}


// Set the error return region to the zoid, and set the error type.
//
void
DRCtestDesc::seterr(const Zoid *z, int erc)
{
    td_result = erc;
    td_pbad[0].set(z->xll, z->yl);
    td_pbad[1].set(z->xul, z->yu);
    td_pbad[2].set(z->xur, z->yu);
    td_pbad[3].set(z->xlr, z->yl);
    td_pbad[4] = td_pbad[0];
}


// Set the error return region to the passed polygon, and set the
// error type.
//
void
DRCtestDesc::seterr(const Point *p, int erc)
{
    td_result = erc;
    for (int i = 0; i < 5; i++)
        td_pbad[i].set(p[i]);
}


// Set the error return region to the BB of the poly, and set the
// error type.
//
void
DRCtestDesc::seterr(const Point *pts, int numpts, int erc)
{
    td_result = erc;
    td_pbad[0].set(pts->x, pts->y);
    td_pbad[2] = td_pbad[0];
    int num = numpts - 2;
    pts++;
    while (num--) {
        if (td_pbad[2].y < pts->y)
            td_pbad[2].y = pts->y;
        if (td_pbad[0].y > pts->y)
            td_pbad[0].y = pts->y;
        if (td_pbad[2].x < pts->x)
            td_pbad[2].x = pts->x;
        if (td_pbad[0].x > pts->x)
            td_pbad[0].x = pts->x;
        pts++;
    }
    td_pbad[1].set(td_pbad[0].x, td_pbad[2].y);
    td_pbad[3].set(td_pbad[2].x, td_pbad[0].y);
    td_pbad[4] = td_pbad[0];
}


// Set the error zoid to a box around the point, and set the error type.
//
void
DRCtestDesc::seterr_pt(const Point *p, int erc)
{
    int dd;
    if (td_type == drMinArea || td_type == drMaxArea || td_type == drNoHoles
            || dimen() == 0)
        dd = INTERNAL_UNITS(0.5);
    else
        dd = dimen();
    td_result = erc;
    td_pbad[0].set(p->x - dd, p->y - dd);
    td_pbad[1].set(p->x - dd, p->y + dd);
    td_pbad[2].set(p->x + dd, p->y + dd);
    td_pbad[3].set(p->x + dd, p->y - dd);
    td_pbad[4] = td_pbad[0];
}


//
// Static test functions.
//

// Static function.
// Callback for intrinsic tests.
//
XIrt
DRCtestDesc::testEmpty(Zoid *ZB, DRCtestDesc *test, DRCedgeEval*, bool *istrue)
{
    int fudge = test->td_fudge;
    sLspec *lspec = test->hasTarget() ? &test->td_target : &test->td_source;
    if (test->td_testdim <= fudge)
        return (XIbad);
    *istrue = true;
    if (ZB->yu - ZB->yl <= fudge)
        return (XIok);
    SIlexprCx cx(0, CDMAXCALLDEPTH, ZB);
    CovType cov;
    XIrt ret = lspec->testZlistCovNone(&cx, &cov, fudge);
    if (ret == XIok && cov != CovNone)
        *istrue = false;
    return (ret);
}


// Static function.
// Callback for intrinsic tests.
//
XIrt
DRCtestDesc::testFull(Zoid *ZB, DRCtestDesc *test, DRCedgeEval *ev,
    bool *istrue)
{
    int fudge = test->td_fudge;
    sLspec *lspec = test->hasTarget() ? &test->td_target : &test->td_source;
    if (test->td_testdim <= fudge)
        return (XIbad);
    *istrue = true;
    if (ZB->yu - ZB->yl <= fudge)
        return (XIok);
    if (!test->hasTarget() && test->td_where == DRCinside) {
        // "minWidth" test, this may speed things up
        bool cc;
        if (ZB->test_coverage(ev->epoly()->zlist(), &cc, fudge) && cc)
            return (XIok);
    }
    SIlexprCx cx(0, CDMAXCALLDEPTH, ZB);
    CovType cov;
    XIrt ret = lspec->testZlistCovFull(&cx, &cov, fudge);
    if (ret == XIok && (cov != CovFull))
        *istrue = false;
    return (ret);
}


// Static function.
// Callback for intrinsic tests.
//
XIrt
DRCtestDesc::testNotFull(Zoid *ZB, DRCtestDesc *test, DRCedgeEval*,
    bool *istrue)
{
    int fudge = test->td_fudge;
    sLspec *lspec = test->hasTarget() ? &test->td_target : &test->td_source;
    if (test->td_testdim <= fudge)
        return (XIbad);
    *istrue = true;
    if (ZB->yu - ZB->yl <= fudge)
        return (XIok);
    SIlexprCx cx(0, CDMAXCALLDEPTH, ZB);
    CovType cov;
    XIrt ret = lspec->testZlistCovFull(&cx, &cov, fudge);
    if (ret == XIok && (cov == CovFull))
        *istrue = false;
    return (ret);
}


// Static function.
// Callback for intrinsic edge length test.
//
XIrt
DRCtestDesc::testEdgeLen(Zoid *ZB, DRCtestDesc *test, DRCedgeEval*,
    bool *istrue)
{
    *istrue = true;
    if (ZB->xll == ZB->xur) {
        int l = abs(ZB->yu - ZB->yl);
        if (l < test->td_testdim) {
            ZB->xlr += 500;
            ZB->xur = ZB->xlr;
            ZB->xll -= 500;
            ZB->xul = ZB->xll;
            *istrue = false;
        }
    }
    else if (ZB->yl == ZB->yu) {
        int l = abs(ZB->xur - ZB->xll);
        if (l < test->td_testdim) {
            ZB->yu += 500;
            ZB->yl -= 500;
            *istrue = false;
        }
    }
    else {
        int dx = ZB->xul - ZB->xll;
        int dy = ZB->yu - ZB->yl;
        int l = mmRnd(sqrt(dx*(double)dx + dy*(double)dy));
        if (l < test->td_testdim) {
            ZB->xlr += 500;
            ZB->xur = ZB->xlr;
            ZB->xll -= 500;
            ZB->xul = ZB->xll;
            *istrue = false;
        }
    }
    return (XIok);
}


// Static function.
// "Test function" to simply obtain the edge length (export).
//
XIrt
DRCtestDesc::getEdgeLen(Zoid *ZB, DRCtestDesc*, DRCedgeEval *ev, bool *istrue)
{
    *istrue = true;
    if (ZB->xll == ZB->xur)
        ev->setIntReturn(abs(ZB->yu - ZB->yl));
    else if (ZB->yl == ZB->yu)
        ev->setIntReturn(abs(ZB->xur - ZB->xll));
    else {
        double dx = ZB->xul - ZB->xll;
        double dy = ZB->yu - ZB->yl;
        ev->setIntReturn(mmRnd(sqrt(dx*dx + dy*dy)));
    }
    return (XIok);
}


// Static function.
// "Test function" to simply add the test region to a list (export).
//
XIrt
DRCtestDesc::getZlist(Zoid *ZB, DRCtestDesc*, DRCedgeEval *ev, bool *istrue)
{
    Zlist *zl = new Zlist(ZB);
    ev->addAccumZlist(zl);
    *istrue = true;
    return (XIok);
}


//
// Remaining DRCtestDesc functions are private.
//

namespace {
    // Return true if we have reached the terminal error count for the
    // given level and rule.
    //
    inline bool errbrk(DRCtestDesc *td, DRCerrCx &errs)
    {
        if (!errs.has_errs())
            return (false);
        if (DRC()->errorLevel() == 0)
            return (true);
        if (DRC()->errorLevel() == 1 && errs.is_flag(td->type()))
            return (true);
        return (false);
    }
}


DRCtestDesc *
DRCtestDesc::testPoly_NOOP(PolyObj*, const CDl*, DRCerrCx&, XIrt *eret)
{
    *eret = XIok;
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_FAIL(PolyObj *po, const CDl*, DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (!po)
        return (td_next);
    if (po->odesc()) {
        Zoid Z(&po->odesc()->oBB());
        seterr(&Z, TT_EX);
    }
    else {
        BBox BB(po->points(), po->numpts());
        Zoid Z(&BB);
        seterr(&Z, TT_EX);
    }
    errs.add(this, 0);
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_Overlap(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (ldtarget && skipThis(ldtarget))
        return (td_next);
    if (!dpo || !dpo->zlist())
        return (td_next);

    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        PolyList *p0 = expandRegions(dpo);
        for (PolyList *p = p0; p; p = p->next) {
            Zlist *zl = p->po.toZlist();
            bool istrue;
            XIrt ret = testOverlap(zl, &istrue);
            Zlist::destroy(zl);
            if (ret != XIok) {
                *eret = ret;
                if (ret == XIbad)
                    treeError();
                PolyList::destroy(p0);
                return (0);
            }
            if (!istrue)
                errs.add(this, 0);
            if (errbrk(this, errs))
                break;
        }
        PolyList::destroy(p0);
    }
    else {
        bool istrue;
        XIrt ret = testOverlap(dpo->zlist(), &istrue);
        if (ret != XIok) {
            *eret = ret;
            if (ret == XIbad)
                treeError();
            return (0);
        }
        if (!istrue)
            errs.add(this, 0);
    }
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_IfOverlap(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (ldtarget && skipThis(ldtarget))
        return (td_next);
    if (!dpo || !dpo->zlist())
        return (td_next);

    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        PolyList *p0 = expandRegions(dpo);
        for (PolyList *p = p0; p; p = p->next) {
            Zlist *zl = p->po.toZlist();
            bool istrue;
            XIrt ret = testIfOverlap(zl, &istrue);
            Zlist::destroy(zl);
            if (ret != XIok) {
                *eret = ret;
                if (ret == XIbad)
                    treeError();
                PolyList::destroy(p0);
                return (0);
            }
            if (!istrue)
                errs.add(this, 0);
            if (errbrk(this, errs))
                break;
        }
        PolyList::destroy(p0);
    }
    else {
        bool istrue;
        XIrt ret = testIfOverlap(dpo->zlist(), &istrue);
        if (ret != XIok) {
            *eret = ret;
            if (ret == XIbad)
                treeError();
            return (0);
        }
        if (!istrue)
            errs.add(this, 0);
    }
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_NoOverlap(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (ldtarget && skipThis(ldtarget))
        return (td_next);
    if (!dpo || !dpo->zlist())
        return (td_next);

    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        PolyList *p0 = expandRegions(dpo);
        for (PolyList *p = p0; p; p = p->next) {
            Zlist *zl = p->po.toZlist();
            bool istrue;
            XIrt ret = testNoOverlap(zl, &istrue);
            Zlist::destroy(zl);
            if (ret != XIok) {
                *eret = ret;
                if (ret == XIbad)
                    treeError();
                PolyList::destroy(p0);
                return (0);
            }
            if (!istrue)
                errs.add(this, 0);
            if (errbrk(this, errs))
                break;
        }
        PolyList::destroy(p0);
    }
    else {
        bool istrue;
        XIrt ret = testNoOverlap(dpo->zlist(), &istrue);
        if (ret != XIok) {
            *eret = ret;
            if (ret == XIbad)
                treeError();
            return (0);
        }
        if (!istrue)
            errs.add(this, 0);
    }
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_AnyOverlap(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (ldtarget && skipThis(ldtarget))
        return (td_next);
    if (!dpo || !dpo->zlist())
        return (td_next);

    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        PolyList *p0 = expandRegions(dpo);
        for (PolyList *p = p0; p; p = p->next) {
            Zlist *zl = p->po.toZlist();
            bool istrue;
            XIrt ret = testAnyOverlap(zl, &istrue);
            Zlist::destroy(zl);
            if (ret != XIok) {
                *eret = ret;
                if (ret == XIbad)
                    treeError();
                PolyList::destroy(p0);
                return (0);
            }
            if (!istrue)
                errs.add(this, 0);
            if (errbrk(this, errs))
                break;
        }
        PolyList::destroy(p0);
    }
    else {
        bool istrue;
        XIrt ret = testAnyOverlap(dpo->zlist(), &istrue);
        if (ret != XIok) {
            *eret = ret;
            if (ret == XIbad)
                treeError();
            return (0);
        }
        if (!istrue)
            errs.add(this, 0);
    }
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_PartOverlap(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (ldtarget && skipThis(ldtarget))
        return (td_next);
    if (!dpo || !dpo->zlist())
        return (td_next);

    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        PolyList *p0 = expandRegions(dpo);
        for (PolyList *p = p0; p; p = p->next) {
            Zlist *zl = p->po.toZlist();
            bool istrue;
            XIrt ret = testPartOverlap(zl, &istrue);
            Zlist::destroy(zl);
            if (ret != XIok) {
                *eret = ret;
                if (ret == XIbad)
                    treeError();
                PolyList::destroy(p0);
                return (0);
            }
            if (!istrue)
                errs.add(this, 0);
            if (errbrk(this, errs))
                break;
        }
        PolyList::destroy(p0);
    }
    else {
        bool istrue;
        XIrt ret = testPartOverlap(dpo->zlist(), &istrue);
        if (ret != XIok) {
            *eret = ret;
            if (ret == XIbad)
                treeError();
            return (0);
        }
        if (!istrue)
            errs.add(this, 0);
    }
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_AnyNoOverlap(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (ldtarget && skipThis(ldtarget))
        return (td_next);
    if (!dpo || !dpo->zlist())
        return (td_next);

    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        PolyList *p0 = expandRegions(dpo);
        for (PolyList *p = p0; p; p = p->next) {
            Zlist *zl = p->po.toZlist();
            bool istrue;
            XIrt ret = testAnyNoOverlap(zl, &istrue);
            Zlist::destroy(zl);
            if (ret != XIok) {
                *eret = ret;
                if (ret == XIbad)
                    treeError();
                PolyList::destroy(p0);
                return (0);
            }
            if (!istrue)
                errs.add(this, 0);
            if (errbrk(this, errs))
                break;
        }
        PolyList::destroy(p0);
    }
    else {
        bool istrue;
        XIrt ret = testAnyNoOverlap(dpo->zlist(), &istrue);
        if (ret != XIok) {
            *eret = ret;
            if (ret == XIbad)
                treeError();
            return (0);
        }
        if (!istrue)
            errs.add(this, 0);
    }
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_MinArea(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (ldtarget && skipThis(ldtarget))
        return (td_next);
    if (!dpo || !dpo->zlist())
        return (td_next);

    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        PolyList *p0 = expandRegions(dpo);
        for (PolyList *p = p0; p; p = p->next) {
            Zlist *zl = p->po.toZlist();
            if (1.0001*Zlist::area(zl) < area()) {
                seterr(p->po.points, p->po.numpts, TT_AMIN);
                errs.add(this, 0);
            }
            if (errbrk(this, errs))
                break;
            Zlist::destroy(zl);
        }
        PolyList::destroy(p0);
    }
    else {
        if (1.0001*Zlist::area(dpo->zlist()) < area()) {
            // This is pretty ugly.  If the object fails the area test,
            // need to check if it is part of a group that would pass.

            int minw = 0;
            for (DRCtestDesc *td = *tech_prm(td_source.ldesc())->rules_addr();
                    td; td = td->next()) {
                if (td->type() == drMinWidth && !td->hasRegion()) {
                    minw = td->dimen();
                    break;
                }
            }
            int dim;
            if (minw > 0)
                dim = INTERNAL_UNITS(area()/MICRONS(minw));
            else
                dim = INTERNAL_UNITS(sqrt(area()));
            BBox BB;
            Zlist::BB(dpo->zlist(), BB);
            BB.bloat(dim);
            sPF gen(CurCell(Physical), &BB, td_source.ldesc(),
                CDMAXCALLDEPTH);
            CDo *odesc;
            Zlist *zx0 = 0;
            while ((odesc = gen.next(false, false)) != 0) {
                Zlist *zx = odesc->toZlist();
                Zlist *ze = zx;
                while (ze->next)
                    ze = ze->next;
                ze->next = zx0;
                zx0 = zx;
                delete odesc;
            }
            zx0 = Zlist::repartition_ni(zx0);
            Zgroup *zg = Zlist::group(zx0);
            double a = 0.0;
            if (zg->num == 1)
                a = Zlist::area(zg->list[0]);
            else {
                for (int i = 0; i < zg->num; i++) {
                    if (Zlist::intersect(zg->list[i], &dpo->zlist()->Z,
                            false)) {
                        a = Zlist::area(zg->list[i]);
                        break;
                    }
                }
            }
            if (1.0001*a < area()) {
                seterr(dpo->points(), dpo->numpts(), TT_AMIN);
                errs.add(this, 0);
            }
            delete zg;
        }
    }
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_MaxArea(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    *eret = XIok;
    if (td_inhibit || DRC()->ruleDisabled(type()))
        return (td_next);
    if (ldtarget && skipThis(ldtarget))
        return (td_next);
    if (!dpo || !dpo->zlist())
        return (td_next);

    if (td_source.tree() || (td_source.ldesc() != td_objlayer)) {
        PolyList *p0 = expandRegions(dpo);
        for (PolyList *p = p0; p; p = p->next) {
            Zlist *zl = p->po.toZlist();
            if (0.9999*Zlist::area(zl) > area()) {
                seterr(p->po.points, p->po.numpts, TT_AMAX);
                errs.add(this, 0);
            }
            if (errbrk(this, errs))
                break;
            Zlist::destroy(zl);
        }
        PolyList::destroy(p0);
    }
    else {
        if (0.9999*Zlist::area(dpo->zlist()) > area()) {
            seterr(dpo->points(), dpo->numpts(), TT_AMAX);
            errs.add(this, 0);
        }
    }
    return (td_next);
}


DRCtestDesc *
DRCtestDesc::testPoly_EdgeTest(PolyObj *dpo, const CDl *ldtarget,
    DRCerrCx &errs, XIrt *eret)
{
    // The edge tests must always appear at the end of the rule
    // list.  This function will evaluate all of the edge rules,
    // and returns 0 to signal completion of tests.

    *eret = XIok;
    if (ldtarget) {
        for (DRCtestDesc *td = this; td; td = td->next()) {
            if (td->skipThis(ldtarget))
                td->td_inhibit |= 2;
        }
    }

    // first, the rules without a source.tree
    errs.add(testPolyEdges(dpo, eret));

    if (*eret == XIok && (DRC()->errorLevel() > 0 || !errs.has_errs())) {
        for (DRCtestDesc *td = this; td; td = td->next()) {
            if (td->td_inhibit || DRC()->ruleDisabled(td->type()))
                continue;
            if (td->td_source.tree() ||
                    (td->td_source.ldesc() != td->td_objlayer)) {
                PolyList *p0 = td->expandRegions(dpo);
                for (PolyList *p = p0; p; p = p->next) {
                    PolyObj xpo(p->po, false);
                    errs.add(td->testPolyEdges(&xpo, eret, true));
                    if (errbrk(td, errs) || *eret != XIok)
                        break;
                }
                PolyList::destroy(p0);
                if ((DRC()->errorLevel() == 0 && errs.has_errs()) ||
                        *eret != XIok)
                    break;
            }
        }
    }
    if (ldtarget) {
        for (DRCtestDesc *td = this; td; td = td->next())
            td->td_inhibit &= ~2;
    }
    return (0);
}


// Return false if any zoid in the list is not completely covered by
// lspec area
//
XIrt
DRCtestDesc::testOverlap(const Zlist *zl, bool *istrue)
{
    *istrue = true;
    if (!zl)
        return (XIok);
    SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
    CovType cov;
    XIrt ret = td_target.testZlistCovFull(&cx, &cov, td_fudge);
    if (ret != XIok)
        return (ret);
    if (cov == CovFull)
        return (XIok);
    *istrue = false;
    BBox BB;
    Zlist::BB(zl, BB);
    Zoid Z(&BB);
    seterr(&Z, TT_OVL);
    return (XIok);
}


// Return false if zoids in list are all not either totally covered or
// uncovered by lspec area.
//
XIrt
DRCtestDesc::testIfOverlap(const Zlist *zl, bool *istrue)
{
    *istrue = true;
    if (!zl)
        return (XIok);
    SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
    CovType cov;
    XIrt ret = td_target.testZlistCovPartial(&cx, &cov, td_fudge);
    if (ret != XIok)
        return (ret);
    if (cov != CovPartial)
        return (XIok);
    *istrue = false;
    BBox BB;
    Zlist::BB(zl, BB);
    Zoid Z(&BB);
    seterr(&Z, TT_IFOVL);
    return (XIok);
}


// If a dark area of lspec overlaps any zoid in the list, return false.
//
XIrt
DRCtestDesc::testNoOverlap(const Zlist *zl, bool *istrue)
{
    *istrue = true;
    if (!zl)
        return (XIok);
    SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
    CovType cov;
    XIrt ret = td_target.testZlistCovNone(&cx, &cov, td_fudge);
    if (ret != XIok)
        return (ret);
    if (cov == CovNone)
        return (XIok);
    *istrue = false;
    BBox BB;
    Zlist::BB(zl, BB);
    Zoid Z(&BB);
    seterr(&Z, TT_NOOVL);
    return (XIok);
}


// Return false if all zoids are completely uncovered.
//
XIrt
DRCtestDesc::testAnyOverlap(const Zlist *zl, bool *istrue)
{
    *istrue = true;
    if (!zl)
        return (XIok);
    SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
    CovType cov;
    XIrt ret = td_target.testZlistCovNone(&cx, &cov, td_fudge);
    if (ret != XIok)
        return (ret);
    if (cov != CovNone)
        return (XIok);
    *istrue = false;
    BBox BB;
    Zlist::BB(zl, BB);
    Zoid Z(&BB);
    seterr(&Z, TT_ANYOVL);
    return (XIok);
}


// Return false if all zoids are totally covered or uncovered.
//
XIrt
DRCtestDesc::testPartOverlap(const Zlist *zl, bool *istrue)
{
    *istrue = true;
    if (!zl)
        return (XIok);
    SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
    CovType cov;
    XIrt ret = td_target.testZlistCovPartial(&cx, &cov, td_fudge);
    if (ret != XIok)
        return (ret);
    if (cov == CovPartial)
        return (XIok);
    *istrue = false;
    BBox BB;
    Zlist::BB(zl, BB);
    Zoid Z(&BB);
    seterr(&Z, TT_PARTOVL);
    return (XIok);
}


// Return false if all zoids are completely covered.
//
XIrt
DRCtestDesc::testAnyNoOverlap(const Zlist *zl, bool *istrue)
{
    *istrue = true;
    if (!zl)
        return (XIok);
    SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
    CovType cov;
    XIrt ret = td_target.testZlistCovFull(&cx, &cov, td_fudge);
    if (ret != XIok)
        return (ret);
    if (cov != CovFull)
        return (XIok);
    *istrue = false;
    BBox BB;
    Zlist::BB(zl, BB);
    Zoid Z(&BB);
    seterr(&Z, TT_ANOOVL);
    return (XIok);
}


namespace {
    // Struct to hold some needed stuff for error reporting.
    //
    struct ething
    {
        ething(DRCerrCx *e) { errs = e; rule = 0; }

        DRCerrCx *errs;
        DRCtestDesc *rule;
    };


    // Error callback for edge evaluation.
    //
    bool
    err_rpt(DRCtestDesc*, int indx, void *arg)
    {
        ething *e = (ething*)arg;
        e->errs->add(e->rule, indx);
        if (DRC()->errorLevel() < 0)
            return (false);
        return (true);
    }
}


DRCerrRet *
DRCtestDesc::testPolyEdges(PolyObj *dpo, XIrt *eret, bool thisonly)
{
    *eret = XIok;
    DRCedgeEval ev(dpo);
    if (!ev.epoly())
        return (0);

    DRC()->setObjPoints(dpo->points(), dpo->numpts());
    DRCerrCx errs;
    ething earg(&errs);
    while (ev.advance()) {
        DRC()->setObjCurVertex(ev.passcnt() ? ev.passcnt() - 1 : 0);
        bool init_called = false;
        for (DRCtestDesc *td = this; td; td = td->next()) {
            if (td->td_inhibit || DRC()->ruleDisabled(td->type()))
                continue;
            if (!thisonly && (td->td_source.tree() ||
                    (td->td_source.ldesc() != td->td_objlayer)))
                continue;
            if (DRC()->errorLevel() == 1 && errs.is_flag(td->td_type))
                continue;

            if (!init_called) {
                init_called = true;
                // All or the rules on this pass will have the same source.
                XIrt ret = td->initEdgeList(&ev);
                if (ret != XIok) {
                    *eret = ret;
                    if (ret == XIbad)
                        td->treeError();
                    return (errs.get_errs());
                }
            }

            earg.rule = td;
            if (ev.passcnt() == 1) {
                XIrt ret = td->polyEdgesPreTest(&ev, err_rpt, &earg);
                if (ret != XIok) {
                    *eret = ret;
                    if (ret == XIbad)
                        td->treeError();
                    return (errs.get_errs());
                }
            }

            XIrt ret = td->polyEdgesTest(&ev, err_rpt, &earg);
            if (ret != XIok) {
                *eret = ret;
                if (ret == XIbad)
                    td->treeError();
                return (errs.get_errs());
            }
            if (DRC()->errorLevel() == 0 && errs.has_errs())
                return (errs.get_errs());
            if (thisonly)
                break;
        }
        if (!init_called)
            break;
    }
    return (errs.get_errs());
}


// Return a list of polygons consisting of the source.tree regions
// that intersect with the supplied poly.
//
PolyList *
DRCtestDesc::expandRegions(PolyObj *drcpo)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (0);
    Zlist *z0;
    SIlexprCx cx(cursdp, CDMAXCALLDEPTH, drcpo->zlist());
    if (td_source.getZlist( &cx, &z0) == XIok)
        return (Zlist::to_poly_list(z0));
    return (0);
}


// Return true if the 5-point polygon is completely covered by the
// source (for corner overlap test).
//
XIrt
DRCtestDesc::testPolyCovered(Point *po, DRCedgeEval *ev, bool *ret)
{
    *ret = true;
    Zlist *zx0 = Point::toZlist(po, 5);
    if (DRC()->isShowZoids() && DSP()->MainWdesc())
        Zlist::show(zx0);
    for (Zlist *zx = zx0; zx; zx = zx->next) {
        if (zx->Z.yu - zx->Z.yl <= td_fudge)
            continue;

        bool istrue;
        XIrt xrt = (*td_testfunc)(&zx->Z, this, ev, &istrue);
        if (xrt != XIok) {
            Zlist::destroy(zx0);
            return (xrt);
        }
        if (!istrue) {
            *ret = false;
            break;
        }
    }
    Zlist::destroy(zx0);
    return (XIok);
}


// A variation on testEmpty that returns the overlap along the edge,
//
XIrt
DRCtestDesc::spaceTest(Zoid *ZB, DRCedgeEval *ev, int *edge_length)
{
    sLspec *lspec = hasTarget() ? &td_target : &td_source;
    if (td_testdim <= td_fudge)
        return (XIbad);
    *edge_length = 0;
    if (ZB->yu - ZB->yl <= td_fudge)
        return (XIok);

    SIlexprCx cx(0, CDMAXCALLDEPTH, ZB);
    Zlist *z0;
    XIrt ret = lspec->getZlist(&cx, &z0);
    if (ret != XIok)
        return (ret);
    if (z0)
        z0 = Zlist::filter_drc_slivers(z0, td_fudge);
    if (!z0) {
        *edge_length = 0;
        return (ret);
    }
    z0 = Zlist::repartition_ni(z0);
    // The edge is Manhattan.
    int el = 0;
    if (ev->edgecode() == DRCeB) {
        for (Zlist *z = z0; z; z = z->next)
            el += z->Z.xur - z->Z.xul;
    }
    else if (ev->edgecode() == DRCeT) {
        for (Zlist *z = z0; z; z = z->next)
            el += z->Z.xlr - z->Z.xll;
    }
    else {
        for (Zlist *z = z0; z; z = z->next)
            el += z->Z.yu - z->Z.yl;
    }
    Zlist::destroy(z0);
    *edge_length = el;
    return (XIok);
}
// End of DRCtestDesc functions.


DRCedgeEval::DRCedgeEval(PolyObj *dpo)
{
    if (dpo && !dpo->setcw())
        dpo = 0;
    e_vtx = 0;
    e_elist = 0;
    e_epoly = dpo;
    e_accum_zlist = 0;
    e_int_return = 0;
    e_passcnt = 0;
    e_edgecode = 0;
    e_ang = 0;
    e_cw = dpo ? dpo->iscw() : false;
    e_last_vtx = false;
    e_check_wire_edge = false;
    e_wire_checked = false;
    if (dpo) {
        e_vtx = dpo->points();
        e_p1.set(*e_vtx);
        e_p2 = e_p1;
        e_p3 = e_p2;
    }
}


DRCedgeEval::~DRCedgeEval()
{
    Zlist::destroy(e_elist);
    Zlist::destroy(e_accum_zlist);
}


// Set the context to the first/next edge.  Iterate through edges by
// calling until false is returned.  Since info for two edges is
// required for the corner tests, no corner test is performed on the
// first iteration, and only a corner test is performed on the last
// iteration.
//
bool
DRCedgeEval::advance()
{
    if (e_last_vtx)
        return (false);
    Zlist::destroy(e_elist);
    e_elist = 0;
    if (e_passcnt == e_epoly->numpts() - 1) {
        e_vtx = e_epoly->points() + 1;
        e_passcnt = 0;
        e_last_vtx = true;
    }
    else {
        e_vtx++;
        e_passcnt++;
    }
    e_p1 = e_p2;
    e_p2 = e_p3;
    e_p3.set(*e_vtx);
    return (true);
}


void
DRCedgeEval::addAccumZlist(Zlist *zl)
{
    zl->next = e_accum_zlist;
    e_accum_zlist = zl;
}


// Initialize the angle parameter.
//
void
DRCedgeEval::initAng()
{
    if (e_passcnt != 1)
        e_ang = DRC()->outsideAngle(&e_p1, &e_p2, &e_p3, e_cw);
}


namespace {
    // Return false if a width test is needed.
    //
    bool
    checkWire(CDw *wd, int dimen)
    {
        if (mmRnd(wd->wire_width()) < dimen)
            // always check these
            return (false);

        // The edge test can be skipped if there is more than one segment,
        // or if ends project.  Have to further test length of single-segment
        // CDWIRE_FLUSH wires.
        //
        if (wd->numpts() > 2 || wd->wire_style() != CDWIRE_FLUSH)
            return (true);
        if (wd->numpts() == 2) {
            const Point *pts = wd->points();
            // If length > min dimension, skip test.
            if (abs(pts[1].x - pts[0].x) >= dimen ||
                    abs(pts[1].y - pts[0].y) >= dimen)
                return (true);
            if ((pts[1].x - pts[0].x) * (double)(pts[1].x - pts[0].x) +
                    (pts[1].y - pts[0].y) * (double)(pts[1].y - pts[0].y) >=
                    dimen * (double)dimen)
                return (true);
        }
        else {
            // Single vertex, this is bad.
            wd->set_wire_style(CDWIRE_EXTEND);
        }
        return (false);
    }
}


bool
DRCedgeEval::wireCheck(int dim)
{
    bool iswire = (epoly()->odesc() && epoly()->odesc()->type() == CDWIRE);
    if (!e_wire_checked) {
        if (iswire) {
            if (!checkWire((CDw*)epoly()->odesc(), dim))
                e_check_wire_edge = true;
        }
        e_wire_checked = true;
    }
    return (iswire);
}

