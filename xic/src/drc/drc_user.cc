
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
 $Id: drc_user.cc,v 5.39 2017/03/19 01:58:35 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "drc.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "tech.h"
#include "errorlog.h"


// User Defined Design Rules
//
// Definition, ahead of layers in technology file
//
//  DrcTest  testname  a b c d ...
//  Edge Outside/Inside expression           | 0 or more of these
//  MinEdge dimen                            | edge filter
//  MaxEdge dimen                            | edge filter
//  Test Outside/Inside dimension expression | 1 or more of these
//  TestCornerOverlap dimen                  | enable corner overlap  test
//  Evaluate logical_expression              | see note
//   or
//  Evaluate
//   script lines
//  EndScript
//
//  End
//
// note:
// The logical_expression must set a variable named "fail", and
// involves logical operations of the functions "DRCuserTest(n)", where
// n is a 0-based index to the Test specifications in order of
// appearance.  In the block form, setting a variable named "fail"
// indicates that the test failed.
//
// The "arguments" a b c d represent replaceable tokens, and are
// used in the expressions in the form %a%, etc.  Thus, an argument
// can represent a layername or a dimension.

// in layer block:
//
//  testname a b c d ...
//
// The a b c d are layer names, dimensions, etc used as parameters
// for the rule.  The testname is the keyword defined in the
// definition block, and must be unique among keywords.


// This char keys formal/actual substitution in expressions
#define SUBCHR '%'

namespace {
    char *subst(const char*, char**, char**, int);
}

namespace drc_user {
    // structures to hold evaluation context for texts
    //
    struct sTest
    {
        DRCtestCnd *test;
        bool called;        // set true on first call
        bool result;        // set true if on return DRCtestDesc::result == 0
    };

    struct TestState
    {
        TestState(DRCtestDesc *td, DRCedgeEval *e, DRCtest *u)
            {
                ts_rule = td;
                ts_ev = e;
                ts_user = u;
                ts_eval = 0;
                ts_zl = 0;
                int i = 0;
                for (DRCtestCnd *t = ts_user->tests(); t; t = t->next())
                    i++;
                ts_numtests = i;
                ts_tests = new sTest[i];

                i = 0;
                for (DRCtestCnd *t = ts_user->tests(); t; t = t->next()) {
                    ts_tests[i].test = t;
                    ts_tests[i].called = false;
                    ts_tests[i].result = false;
                    i++;
                }
            }

        ~TestState()
            {
                delete [] ts_tests;
            }

        XIrt edgeTest(int);
        XIrt cornerTest(int);
        XIrt cornerOverlapTest(int);
        int encode();

        XIrt userTest(int testnum, bool *istrue)
            {
                ts_rule->setTestFunc(DRCtestDesc::testEmpty);
                XIrt ret = (this->*(this->ts_eval))(testnum);
                if (ret == XIok)
                    *istrue = !ts_tests[testnum].result;
                return (ret);
            }

        XIrt userEmpty(int testnum, bool *istrue)
            {
                ts_rule->setTestFunc(DRCtestDesc::testEmpty);
                XIrt ret = (this->*(this->ts_eval))(testnum);
                if (ret == XIok)
                    *istrue = ts_tests[testnum].result;
                return (ret);
            }

        XIrt userFull(int testnum, bool *istrue)
            {
                ts_rule->setTestFunc(DRCtestDesc::testFull);
                XIrt ret = (this->*(this->ts_eval))(testnum);
                if (ret == XIok)
                    *istrue = ts_tests[testnum].result;
                return (ret);
            }

        XIrt userZlist(int testnum, Zlist **zret)
            {
                ts_rule->setTestFunc(DRCtestDesc::getZlist);
                XIrt ret = (this->*(this->ts_eval))(testnum);
                if (ret == XIok)
                    *zret = ts_ev->accumZlist();
                else {
                    Zlist *zl = ts_ev->accumZlist();
                    Zlist::destroy(zl);
                }
                return (ret);
            }

        XIrt userEdgeLength(int testnum, double *length)
            {
                ts_rule->setTestFunc(DRCtestDesc::getEdgeLen);
                XIrt ret = (this->*(this->ts_eval))(testnum);
                if (ret == XIok)
                    *length = MICRONS(ts_ev->intReturn());
                return (ret);
            }

        sTest &tests(int i)             { return (ts_tests[i]); }
        int numtests()                  { return (ts_numtests); }
        const Zlist *zlist()            { return (ts_zl); }
        void setZlist(const Zlist *z)   { ts_zl = z; }

        void set_eval(XIrt(TestState::*f)(int)) { ts_eval = f; }

    private:
        XIrt (TestState::*ts_eval)(int);
        DRCtestDesc *ts_rule;
        DRCedgeEval *ts_ev;
        DRCtest *ts_user;
        const Zlist *ts_zl;
        int ts_numtests;
        sTest *ts_tests;
    };
}


//=====================================================
// The following are implementations of the user test
// script functions for user defined rules
//=====================================================

// If test region not empty return true.
//
//
XIrt
cDRC::userTest(int testnum, bool *istrue)
{
    *istrue = true;
    if (!drc_test_state)
        return (XIbad);
    return (drc_test_state->userTest(testnum, istrue));
}


// If test region empty return true.
//
XIrt
cDRC::userEmpty(int testnum, bool *istrue)
{
    *istrue = true;
    if (!drc_test_state)
        return (XIbad);
    return (drc_test_state->userEmpty(testnum, istrue));
}


// If test region fully covered return true.
//
XIrt
cDRC::userFull(int testnum, bool *istrue)
{
    *istrue = true;
    if (!drc_test_state)
        return (XIbad);
    return (drc_test_state->userFull(testnum, istrue));
}


// Return a list of zoids clipped from the test region.
//
XIrt
cDRC::userZlist(int testnum, Zlist **zret)
{
    *zret = 0;
    if (!drc_test_state)
        return (XIbad);
    return (drc_test_state->userZlist(testnum, zret));
}


// Return the length of the test segment.
//
XIrt
cDRC::userEdgeLength(int testnum, double *length)
{
    *length = 0.0;
    if (!drc_test_state)
        return (XIbad);
    return (drc_test_state->userEdgeLength(testnum, length));
}
// End of script function callbacks
// End of cDRC functions


// Initialize the Edge specification, building parse trees and
// referencing layers.
//
bool
DRCedgeCnd::setup()
{
    const char *str = ec_lspec.lname();
    if (!str) {
        Errs()->add_error("DRCedgeCnd::setup: edge with no string!");
        return (false);
    }
    ec_lspec.set_lname_pointer(0);
    const char *tstr = str;
    bool ret = ec_lspec.parseExpr(&str, true);
    delete [] tstr;
    if (!ret) {
        Errs()->add_error("DRCedgeCnd::setup: expression parse failed");
        return (false);
    }
    if (ec_lspec.lname()) {
        CDl *sld = CDldb()->findLayer(ec_lspec.lname(), Physical);
        if (!sld) {
            Errs()->add_error("DRCedgeCnd::setup: unknown layer %s",
                ec_lspec.lname());
            return (false);
        }
        ec_lspec.set_ldesc(sld);
    }
    else if (ec_lspec.tree()) {
        char *bad = ec_lspec.tree()->checkLayersInTree();
        if (bad) {
            Errs()->add_error("DRCedgeCnd::setup: unknown layer %s",
                bad);
            return (false);
        }
    }
    return (true);
}


// Print the text of the Edge specification to lstr.
//
void
DRCedgeCnd::print(sLstr *lstr)
{
    lstr->add("Edge");
    if (ec_where == DRCoutside)
        lstr->add(" Outside ");
    else
        lstr->add(" Inside ");
    char *s = ec_lspec.string();
    lstr->add(s);
    delete [] s;
}
// End of DRCedgeCnd functions


// Initialize the Test specification, building parse trees and
// referencing layers.
//
char *
DRCtestCnd::init()
{
    const char *str = tc_lspec.lname();
    if (!str)
        return (lstring::copy("Error: Test with no string!"));
    tc_lspec.set_lname_pointer(0);
    const char *tstr = str;
    char *tok = lstring::gettok(&str);
    char buf[64];
    double g = 0.0;
    if (tok) {
        if (sscanf(tok, "%lf", &g) != 1 || g <= 0.0) {
            sprintf(buf, "bad dimension %s", tok);
            delete [] tstr;
            delete [] tok;
            return (lstring::copy(buf));
        }
        delete [] tok;
    }
    tc_dimension = INTERNAL_UNITS(g);
    bool ret = tc_lspec.parseExpr(&str, true);
    delete [] tstr;
    if (!ret) {
        sLstr lstr;
        lstr.add("parse error\n");
        lstr.add(Errs()->get_error());
        return (lstr.string_trim());
    }
    if (tc_lspec.lname()) {
        CDl *sld = CDldb()->findLayer(tc_lspec.lname(), Physical);
        if (!sld) {
            sprintf(buf, "unknown layer %s", tc_lspec.lname());
            return (lstring::copy(buf));
        }
        tc_lspec.set_ldesc(sld);
    }
    else if (tc_lspec.tree()) {
        char *bad = tc_lspec.tree()->checkLayersInTree();
        if (bad) {
            sprintf(buf, "unknown layer %s", bad);
            delete [] bad;
            return (lstring::copy(buf));
        }
    }
    return (0);
}


// Print the text of the Test specification to lstr.
//
void
DRCtestCnd::print(sLstr *lstr)
{
    lstr->add("Test");
    if (tc_where == DRCoutside)
        lstr->add(" Outside ");
    else
        lstr->add(" Inside ");
    char *s = tc_lspec.string();
    lstr->add(s);
    delete [] s;
}
// End of DRCtestCnd functions


DRCtest::~DRCtest()
{
    delete [] t_name;
    for (int i = 0; i < t_argc; i++)
        delete [] t_argv[i];
    delete [] t_argv;
    DRCedgeCnd::destroy(t_edges);
    delete [] t_eminstr;
    delete [] t_emaxstr;
    DRCtestCnd::destroy(t_tests);
    delete [] t_fail;
    if (t_evaltree) {
        delete t_evaltree;
        siVariable::destroy(t_variables);
    }
    else if (t_evalfunc) {
        delete t_evalfunc;
        // The variables are deleted in t_evalfunc.
    }
    Zlist::destroy(t_edgeZlist);
    delete [] t_cotstr;
}


// Perform the formal/actual substitutions in the raw specification
// strings, and copy the elements This is called when the reference
// is parsed.
//
bool
DRCtest::substitute(const DRCtest *tst)
{
    DRCedgeCnd *eend = 0;
    for (DRCedgeCnd *e = tst->t_edges; e; e = e->next()) {
        DRCedgeCnd *tmp = new DRCedgeCnd(*e);
        tmp->setNext(0);
        char *s = subst(e->lspec()->lname(), tst->t_argv, t_argv, t_argc);
        tmp->lspec()->set_lname_pointer(s);
        if (!tmp->lspec()->lname())
            return (false);
        if (!eend)
            eend = t_edges = tmp;
        else {
            eend->setNext(tmp);
            eend = eend->next();
        }
    }
    if (tst->t_eminstr) {
        t_eminstr = subst(tst->t_eminstr, tst->t_argv, t_argv, t_argc);
        if (!t_eminstr)
            return (false);
    }
    if (tst->t_emaxstr) {
        t_emaxstr = subst(tst->t_emaxstr, tst->t_argv, t_argv, t_argc);
        if (!t_emaxstr)
            return (false);
    }

    DRCtestCnd *tend = 0;
    for (DRCtestCnd *t = tst->t_tests; t; t = t->next()) {
        DRCtestCnd *tmp = new DRCtestCnd(*t);
        tmp->setNext(0);
        char *s = subst(t->lspec()->lname(), tst->t_argv, t_argv, t_argc);
        tmp->lspec()->set_lname_pointer(s);
        if (!tmp->lspec()->lname())
            return (false);
        if (!tend)
            tend = t_tests = tmp;
        else {
            tend->setNext(tmp);
            tend = tend->next();
        }
    }
    if (tst->t_cotstr) {
        t_cotstr = subst(tst->t_cotstr, tst->t_argv, t_argv, t_argc);
        if (!t_cotstr)
            return (false);
    }
    t_fail = subst(tst->t_fail, tst->t_argv, t_argv, t_argc);
    return (true);
}


// Initialize the layers/parse trees from the raw specification strings,
// Called when the rules are initialized, after all layers have been
// defined.
//
char *
DRCtest::init()
{
    for (DRCedgeCnd *e = t_edges; e; e = e->next()) {
        if (!e->setup())
            return (lstring::copy(Errs()->get_error()));
    }
    int numtests = 0;
    for (DRCtestCnd *t = t_tests; t; t = t->next(), numtests++) {
        char *err = t->init();
        if (err)
            return (err);
    }
    if (t_eminstr) {
        double d;
        if (sscanf(t_eminstr, "%lf", &d) == 1 && d >= 0.0)
            t_edgemin = INTERNAL_UNITS(d);
        else
            return (lstring::copy("Error: bad MINEDGE specification"));
    }
    if (t_emaxstr) {
        double d;
        if (sscanf(t_emaxstr, "%lf", &d) == 1 && d >= 0.0)
            t_edgemax = INTERNAL_UNITS(d);
        else
            return (lstring::copy("Error: bad MAXEDGE specification"));
    }
    if (t_edgemin > 0 && t_edgemax > 0 && t_edgemin > t_edgemax) {
        int i = t_edgemin;
        t_edgemin = t_edgemax;
        t_edgemax = i;
    }
    if (t_cotstr) {
        double d;
        if (sscanf(t_cotstr, "%lf", &d) == 1 && d >= 0.0)
            t_cotwidth = INTERNAL_UNITS(d);
        else
            return (lstring::copy(
                "Error: bad TESTCORNEROVERLAP specification"));
    }

    const char *s = t_fail;
    if (s) {
        while (isspace(*s))
            s++;
    }
    if (!s || !*s)
        return (lstring::copy("no EVALUATE condition"));

    // If the t_fail text has multiple lines, it is a block.  Otherwise,
    // assume that it is a single expression.  Note that ';' is no good
    // in a single expression.

    bool useblock = false;
    const char *q = strchr(s, '\n');
    if (q) {
        while (isspace(*q))
            q++;
        if (*q)
            useblock = true;
    }

    // set up variables
    setvars(0, useblock, 0);

    siVariable *tv = SIparse()->getVariables();
    SIparse()->setVariables(t_variables);
    if (useblock)
        t_evalfunc = SI()->GetBlock(0, 0, &s, 0);
    else
        t_evaltree = SIparse()->getTree(&s, true);
    SIparse()->setVariables(tv);
    if (!t_evaltree && !t_evalfunc)
        return (lstring::copy("parse error in EVALUATE condition"));
    return (0);
}


// Set the values for user's info variables, or create the variables.
//
void
DRCtest::setvars(int ct, bool useblock, const Zoid *z)
{
    // Set up the predefined variables.  the "fail" variable must be
    // last, i.e., at the list head.
    //
    // New variables are added to the *end* of the list.
    //
    if (t_variables) {
        Variable *v = t_variables;
        // fail
        v->content.value = 0.0;
        if (t_evalfunc) {
            v = v->next;
            // _ObjType
            v->content.value = DRC()->getObjType();
            v = v->next;
            // _ObjNumEdges
            v->content.value = DRC()->getObjNumPoints();
            v = v->next;
            // _CurEdge
            v->content.value = DRC()->getObjCurVertex();
            v = v->next;
            // _CurTest
            v->content.value = ct;
            if (z) {
                v = v->next;
                // _CurY2
                v->content.value = MICRONS(z->yu);
                v = v->next;
                // _CurX2
                v->content.value = MICRONS(z->xur);
                v = v->next;
                // _CurY1
                v->content.value = MICRONS(z->yl);
                v = v->next;
                // _CurX1
                v->content.value = MICRONS(z->xll);
            }
        }
    }
    else {
        // create the variables
        //
        if (useblock) {
            siVariable *v = new siVariable;
            v->name = lstring::copy("_CurX1");
            v->type = TYP_SCALAR;
            v->next = t_variables;
            t_variables = v;

            v = new siVariable;
            v->name = lstring::copy("_CurY1");
            v->type = TYP_SCALAR;
            v->next = t_variables;
            t_variables = v;

            v = new siVariable;
            v->name = lstring::copy("_CurX2");
            v->type = TYP_SCALAR;
            v->next = t_variables;
            t_variables = v;

            v = new siVariable;
            v->name = lstring::copy("_CurY2");
            v->type = TYP_SCALAR;
            v->next = t_variables;
            t_variables = v;

            v = new siVariable;
            v->name = lstring::copy("_CurTest");
            v->type = TYP_SCALAR;
            v->next = t_variables;
            t_variables = v;

            v = new siVariable;
            v->name = lstring::copy("_CurEdge");
            v->type = TYP_SCALAR;
            v->next = t_variables;
            t_variables = v;

            v = new siVariable;
            v->name = lstring::copy("_ObjNumEdges");
            v->type = TYP_SCALAR;
            v->next = t_variables;
            t_variables = v;

            v = new siVariable;
            v->name = lstring::copy("_ObjType");
            v->type = TYP_SCALAR;
            v->next = t_variables;
            t_variables = v;
        }

        siVariable *v = new siVariable;
        v->name = lstring::copy("fail");
        v->type = TYP_SCALAR;
        v->next = t_variables;
        t_variables = v;
    }
}


// Print the text of the user-defined test to lstr.
//
void
DRCtest::print(sLstr *lstr)
{
    lstr->add("DrcTest ");
    lstr->add(t_name);
    for (int i = 0; i < t_argc; i++) {
        lstr->add_c(' ');
        lstr->add(t_argv[i]);
    }
    lstr->add_c('\n');
    Tech()->CommentDump(0, lstr, tBlkRule, t_name, "DrcTest");
    for (DRCedgeCnd *e = t_edges; e; e = e->next()) {
        e->print(lstr);
        Tech()->CommentDump(0, lstr, tBlkRule, t_name, "Edge");
        lstr->add_c('\n');
    }
    if (t_eminstr) {
        lstr->add("MinEdge ");
        lstr->add(t_eminstr);
        lstr->add_c('\n');
    }
    Tech()->CommentDump(0, lstr, tBlkRule, t_name, "MinEdge");
    if (t_emaxstr) {
        lstr->add("MaxEdge ");
        lstr->add(t_emaxstr);
        lstr->add_c('\n');
    }
    Tech()->CommentDump(0, lstr, tBlkRule, t_name, "MaxEdge");
    for (DRCtestCnd *t = t_tests; t; t = t->next()) {
        t->print(lstr);
        lstr->add_c('\n');
    }
    if (t_cotstr) {
        lstr->add("TestCornerOverlap ");
        lstr->add(t_cotstr);
        lstr->add_c('\n');
    }
    Tech()->CommentDump(0, lstr, tBlkRule, t_name, "TestCornerOverlap");
    if (t_fail) {
        lstr->add("Evaluate");
        char *s = t_fail;
        bool block = false;
        while (isspace(*s))
            s++;
        char *t = strchr(s, '\n');
        if (t) {
            while (isspace(*t))
                t++;
            if (*t)
                block = true;
        }
        if (block) {
            lstr->add_c('\n');
            lstr->add(t_fail);
            lstr->add("EndScript\n");
        }
        else {
            lstr->add_c(' ');
            lstr->add(t_fail);
            if (!strchr(t_fail, '\n'))
                lstr->add_c('\n');
        }
    }
    Tech()->CommentDump(0, lstr, tBlkRule, t_name, "Evaluate");
    lstr->add("End\n");
    Tech()->CommentDump(0, lstr, tBlkRule, t_name, "End");
}


// Apply filtering to the edge, return false if the edge length is
// not in the range given.
//
bool
DRCtest::is_edge_ok(const Zoid *z, int code)
{
    if (t_edgemin <= 0 && t_edgemax <= 0)
        return (true);
    int width;
    switch (code) {
    case DRCeB:
    case DRCeT:
        width = z->xlr - z->xll;
        break;
    case DRCeL:
    case DRCeR:
        width = z->yu - z->yl;
        break;
    default:
        double dx = z->xll - z->xlr;
        double dy = z->yu - z->yl;
        width = (int)sqrt(dx*dx + dy*dy);
    }
    if ((t_edgemin <= 0 || width >= t_edgemin) &&
            (t_edgemax <= 0 || width <= t_edgemax))
        return (true);
    return (false);
}


//
// The evaluation functions.
//
// We set up a context for evaluation, then execute the Evaluate script.
// The script calls the DRC()->userTest() function through calls to the
// script function DRCuserTest(test_index).
//

XIrt
DRCtest::edgeTest(DRCtestDesc *td, DRCedgeEval *ev, const Zlist *tlist)
{
    drc_user::TestState st(td, ev, this);
    st.set_eval(&drc_user::TestState::edgeTest);

    for (const Zlist *zl = tlist; zl; zl = zl->next) {
        if (!is_edge_ok(&zl->Z, t_edgecode))
            continue;
        for (int i = 0; i < st.numtests(); i++) {
            st.tests(i).called = false;
            st.tests(i).result = false;
        }

        Zlist zl0(&zl->Z);
        st.setZlist(&zl0);

        SIlexprCx cx;
        setvars(0, false, &zl->Z);
        if (t_evaltree) {
            siVariable *tmp = SIparse()->getVariables();
            SIparse()->setVariables(t_variables);
            siVariable v;
            DRC()->setTestState(&st);
            int ret = t_evaltree->evfunc(t_evaltree, &v, &cx);
            DRC()->setTestState(0);
            SIparse()->setVariables(tmp);
            if (ret != OK)
                return (XIbad);
        }
        else if (t_evalfunc) {
            DRC()->setTestState(&st);
            XIrt ret = SI()->EvalFunc(t_evalfunc, &cx);
            DRC()->setTestState(0);
            if (ret != XIok)
                return (ret);
        }
        td->setResult(0);
        if (t_variables->content.value != 0.0) {
            // this is the "fail" variable
            Zoid ZB;
            Point po[5];
            int d = 0;
            for (int i = 0; i < st.numtests(); i++) {
                if (d < st.tests(i).test->dimension())
                    d = st.tests(i).test->dimension();
            }
            td->setTestDim(d);
            td->edgeSetDimen(ev);
            if (!td->edgeTestArea(ev->p2(), ev->p3(), &zl0.Z, ev->edgecode(),
                    &ZB, po))
                td->seterr(&ZB, TT_UET + st.encode());
            else
                td->seterr(po, TT_UET + st.encode());
            break;
        }
    }
    return (XIok);
}


XIrt
DRCtest::cornerTest(DRCtestDesc *td, DRCedgeEval *ev)
{
    drc_user::TestState st(td, ev, this);
    st.set_eval(&drc_user::TestState::cornerTest);

    SIlexprCx cx;
    setvars(1, false, 0);
    if (t_evaltree) {
        siVariable *tmp = SIparse()->getVariables();
        SIparse()->setVariables(t_variables);
        siVariable v;
        DRC()->setTestState(&st);
        int ret = t_evaltree->evfunc(t_evaltree, &v, &cx);
        DRC()->setTestState(0);
        SIparse()->setVariables(tmp);
        if (ret != OK)
            return (XIbad);
    }
    else if (t_evalfunc) {
        DRC()->setTestState(&st);
        XIrt ret = SI()->EvalFunc(t_evalfunc, &cx);
        DRC()->setTestState(0);
        if (ret != XIok)
            return (ret);
    }
    td->setResult(0);
    if (t_variables->content.value != 0.0) {
        // this is the "fail" variable
        Point po[5];
        int d = 0;
        for (int i = 0; i < st.numtests(); i++) {
            if (d < st.tests(i).test->dimension())
                d = st.tests(i).test->dimension();
        }
        td->setTestDim(d);
        if (!td->cornerTestArea(ev->p1(), ev->p2(), ev->p3(), ev->cw(), po)) {
            if (d <= td->fudge())
                d = 500;
            po[0].set(ev->p2().x - d, ev->p2().y - d);
            po[1].set(ev->p2().x - d, ev->p2().y + d);
            po[2].set(ev->p2().x + d, ev->p2().y + d);
            po[3].set(ev->p2().x + d, ev->p2().y - d);
            po[4] = po[0];
        }
        td->seterr(po, TT_UET + st.encode());
    }
    return (XIok);
}


XIrt
DRCtest::cornerOverlapTest(DRCtestDesc *td, DRCedgeEval *ev)
{
    td->setResult(0);
    XIrt ret = td->cornerOverlapTest(ev, t_cotwidth);
    if (ret != XIok)
        return (ret);
    if (!td->result())
        return (XIok);

    // the overlap is insufficient

    drc_user::TestState st(td, ev, this);
    st.set_eval(&drc_user::TestState::cornerOverlapTest);

    // set up a little box within the figure to perform the
    // conjunction tests in
    int x = (ev->p1().x + ev->p2().x + ev->p3().x)/3;
    int y = (ev->p1().y + ev->p2().y + ev->p3().y)/3;
    Zlist zl;
    zl.Z.xll = x - 5;
    zl.Z.xul = x - 5;
    zl.Z.xlr = x + 5;
    zl.Z.xur = x + 5;
    zl.Z.yu = y + 5;
    zl.Z.yl = y - 5;
    st.setZlist(&zl);

    SIlexprCx cx;
    setvars(2, false, 0);
    if (t_evaltree) {
        siVariable *tmp = SIparse()->getVariables();
        SIparse()->setVariables(t_variables);
        siVariable v;
        DRC()->setTestState(&st);
        int tmpret = t_evaltree->evfunc(t_evaltree, &v, &cx);
        DRC()->setTestState(0);
        SIparse()->setVariables(tmp);
        if (tmpret != OK)
            return (XIbad);
    }
    else if (t_evalfunc) {
        DRC()->setTestState(&st);
        ret = SI()->EvalFunc(t_evalfunc, &cx);
        DRC()->setTestState(0);
        if (ret != XIok)
            return (ret);
    }
    td->setResult(0);
    if (t_variables->content.value != 0.0)
        // this is the "fail" variable
        td->seterr_pt(&ev->p2(), TT_UOT + st.encode());
    return (XIok);
}
// End of DRCtest functions


namespace drc_user {
    // Actually perform the edge evaluation.
    //
    XIrt
    TestState::edgeTest(int testnum)
    {
        if (testnum >= ts_numtests)
            return (XIbad);
        if (ts_tests[testnum].called)
            return (XIok);
        DRCtestCnd *t = ts_tests[testnum].test;
        ts_tests[testnum].called = true;

        sLspec lstmp;
        ts_rule->rotateTarget(&lstmp, t->lspec());
        ts_rule->setTestDim(t->dimension());
        ts_rule->setWhere(t->where());
        XIrt ret = ts_rule->edgeTest(ts_ev, ts_zl);
        if (ret == XIok)
            ts_tests[testnum].result = !ts_rule->result();
        ts_rule->rotateTarget(0, &lstmp);
        lstmp.clear();
        return (ret);
    }


    // Actually perform the corner test.
    //
    XIrt
    TestState::cornerTest(int testnum)
    {
        if (testnum >= ts_numtests)
            return (XIbad);
        if (ts_tests[testnum].called)
            return (XIok);
        DRCtestCnd *t = ts_tests[testnum].test;
        ts_tests[testnum].called = true;

        sLspec lstmp;
        ts_rule->rotateTarget(&lstmp, t->lspec());
        ts_rule->setTestDim(t->dimension());
        ts_rule->setWhere(t->where());
        XIrt ret = ts_rule->cornerTest(ts_ev);
        if (ret == XIok)
            ts_tests[testnum].result = !ts_rule->result();
        ts_rule->rotateTarget(0, &lstmp);
        lstmp.clear();
        return (ret);
    }


    // Actually perform the corner overlap test.
    //
    XIrt
    TestState::cornerOverlapTest(int testnum)
    {
        if (testnum >= ts_numtests)
            return (XIbad);
        if (ts_tests[testnum].called)
            return (XIok);
        DRCtestCnd *t = ts_tests[testnum].test;
        ts_tests[testnum].called = true;

        SIlexprCx cx(0, CDMAXCALLDEPTH, ts_zl);
        CovType cov;
        XIrt ret = t->lspec()->testZlistCovNone(&cx, &cov, ts_rule->fudge());
        if (ret == XIok)
            ts_tests[testnum].result = (cov != CovNone);
        return (ret);
    }


    // Encode the test results into the returned int.
    //
    int
    TestState::encode()
    {
        unsigned int z = 0;
        unsigned int mask = 0x100;
        for (int i = 0; i < ts_numtests; i++) {
            if (ts_tests[i].result)
                z |= mask;
            mask <<= 1;
        }
        z |= ts_numtests;
        return (z);
    }
    // End of TestState functions
}


namespace {
    // Return a copy of line, with formal/actual substitutions made.  The
    // substitutions are made to forms 'SUBCHR'text'SUBCHR', no nesting.
    // return 0 and dump a message on error.
    //
    char *
    subst(const char *line, char **formal, char **actual, int ac)
    {
        if (!line)
            return (0);
        sLstr lstr;
        for (const char *s = line; *s; s++) {
            if (*s == SUBCHR) {
                if (*(s+1) == SUBCHR) {
                    lstr.add_c(*s);
                    s++;
                    continue;
                }
                s++;
                char tbuf[256];
                char *p = tbuf;
                while (*s && *s != SUBCHR)
                    *p++ = *s++;
                *p = 0;
                int i;
                for (i = 0; i < ac; i++) {
                    if (lstring::cieq(tbuf, formal[i]))
                        break;
                }
                if (i < ac) {
                    p = actual[i];
                    while (*p)
                        lstr.add_c(*p++);
                }
                else {
                    Log()->WarningLogV(mh::DRC, "Can't substitute for %s.\n",
                        tbuf);
                    return (0);
                }
            }
            else
                lstr.add_c(*s);
        }
        return (lstr.string_trim());
    }
}

