
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
 $Id: drc.h,v 5.136 2017/04/09 23:25:49 stevew Exp $
 *========================================================================*/

#ifndef DRC_H
#define DRC_H

#include "drcif.h"
#include "si_parsenode.h"
#include "si_lspec.h"

class cCHD;
struct DRCedgeEval;
struct DRCerrCx;
struct DRCerrRet;
struct MenuBox;
struct ParseNode;
struct PolyObj;
struct sLstr;
struct SIfunc;
struct SIlexprCx;
struct siVariable;
struct Zlist;
struct Zgroup;
namespace drc_user { struct TestState; }
namespace drc_results { struct DRCresultParser; }

// DRC variables.
#define VA_Drc                  "Drc"
#define VA_DrcLevel             "DrcLevel"
#define VA_DrcNoPopup           "DrcNoPopup"
#define VA_DrcMaxErrors         "DrcMaxErrors"
#define VA_DrcInterMaxObjs      "DrcInterMaxObjs"
#define VA_DrcInterMaxTime      "DrcInterMaxTime"
#define VA_DrcInterMaxErrors    "DrcInterMaxErrors"
#define VA_DrcInterSkipInst     "DrcInterSkipInst"
#define VA_DrcLayerList         "DrcLayerList"
#define VA_DrcUseLayerList      "DrcUseLayerList"
#define VA_DrcRuleList          "DrcRuleList"
#define VA_DrcUseRuleList       "DrcUseRuleList"
#define VA_DrcChdName           "DrcChdName"
#define VA_DrcChdCell           "DrcChdCell"
#define VA_DrcPartitionSize     "DrcPartitionSize"

// Log file prefix
#define DRC_EFILE_PREFIX "drcerror.log"

// Log file header line prefix.
#define DRC_EFILE_HEADER "Design rule validation of cell"

// Special case for cDRC::printFileHeader 
#define DRC_UNKNOWN_AREA (const BBox*)1

// Name for temporary layer.
#define DRC_TMPLYR  "$drctmp$"

// Grid size when partitioning, default is no gridding.
#define DRC_PART_MIN 20.0
#define DRC_PART_MAX 10000.0
#define DRC_PART_DEF 200.0

// Other parameter defaults and limits.
#define DRC_MAX_ERRS_MIN        0
#define DRC_MAX_ERRS_MAX        100000
#define DRC_MAX_ERRS_DEF        0
#define DRC_INTR_MAX_OBJS_MIN   0
#define DRC_INTR_MAX_OBJS_MAX   100000
#define DRC_INTR_MAX_OBJS_DEF   1000
#define DRC_INTR_MAX_TIME_MIN   0
#define DRC_INTR_MAX_TIME_MAX   30000
#define DRC_INTR_MAX_TIME_DEF   5000
#define DRC_INTR_MAX_ERRS_MIN   0
#define DRC_INTR_MAX_ERRS_MAX   1000
#define DRC_INTR_MAX_ERRS_DEF   100
#define DRC_ERR_LEVEL_DEF       DRCone_err
#define DRC_FUDGE_MIN           0
#define DRC_FUDGE_MAX           10
#define DRC_FUDGE_DEF           2
#define DRC_INTERACTIVE_DEF     false
#define DRC_INTR_NO_ERRMSG_DEF  false
#define DRC_INTR_SKIP_INST_DEF  false

enum DRCwhereType { DRCoutside, DRCinside };
enum DRCpassType { DRCintermediate, DRCfirst, DRClast };

// Edge and corner codes.
enum {
    DRCeL       = 1,
    DRCeB       = 2,
    DRCeR       = 4,
    DRCeT       = 8,
    DRCcLL      = 3,
    DRCcLR      = 6,
    DRCcUL      = 9,
    DRCcUR      = 12,
    DRCeCWA     = 16,
    DRCeCWD     = 17,
    DRCeCCWA    = 18,
    DRCeCCWD    = 19
};

// At corners where the outside angle is ANG_MIN or less ('inside'
// corners) perform a width test.  Perform an edge overlap and corner
// space test if the outside angle is 360-ANG_MIN or more.
//
#define ANG_MIN 140

// Corner test macros.
#define DO_COT(a) (a >= (360-ANG_MIN))
#define DO_CWT(a) (a <= ANG_MIN)
#define DO_CST(a) (a >= (360-ANG_MIN))

// Values for testtype in struct DRCtestDesc and errtype in DRCerrRet.
// For the built-in test, the LSB contains the code starting with TT_OVL.
// For the user-defined tests, the LSB contains the test vector length,
// which is <= 24, plus one of TT_U??.  The rest of the int contains a
// bit field of pass/fail bits from the tests.
//
enum {
    TT_UNKN=0,
    TT_UET=1,
    TT_UCT=25,
    TT_UOT=49,
    TT_OVL=128, TT_IFOVL, TT_NOOVL, TT_ANYOVL, TT_PARTOVL, TT_ANOOVL,
    TT_AMIN, TT_AMAX, TT_ELT, TT_EWT, TT_COT, TT_CWT, TT_EST, TT_CST,
    TT_EX, TT_CON, TT_NOH, TT_OPP
};

// Maximum user-defined rule test vector width.
#define DRC_MAX_USER_TEST_SPECS 24

// Maximum arguments in user-defined rule.
#define DRC_MAX_USER_ARGS 50

// List of 'bad' regions returned from tests, used for display.
//
struct DRCerrList
{
    DRCerrList()
        {
            el_next = 0;
            el_pointer = 0;
            el_stack = 0;
            el_message = 0;
            el_descr = 0;
        }

    ~DRCerrList();

    void addBB(BBox&);
    bool intersect(const BBox&);
    void transf(const DRCerrRet*, const cTfmStack*);
    void showbad(WindowDesc*);
    void showfailed(WindowDesc*);
    void newstack(const sPF*);

    void set_next(DRCerrList *el) { el_next = el; }
    void set_pointer (CDo *o)     { el_pointer = o; }

    void set_message(const char *m)
        {
            char *msg = lstring::copy(m);
            delete [] el_message;
            el_message = msg;
        }

    void set_descr(const char *d)
        {
            char *dsc = lstring::copy(d);
            delete [] el_descr;
            el_descr = dsc;
        }

    void set_pbad(unsigned int i, int x, int y)
        {
            if (i < 5)
                el_pbad[i].set(x, y);
        }

    DRCerrList *next()      const { return (el_next); }
    const CDo *pointer()    const { return (el_pointer); }
    const CDc **stack()     const { return (el_stack); }
    const char *message()   const { return (el_message); }
    const char *descr()     const { return (el_descr); }
    const Point &pbad(unsigned int i)
                            const { return (el_pbad[i < 5 ? i : 4]); }

private:
    DRCerrList *el_next;
    CDo *el_pointer;        // Copy of object which failed test,
                            //  transformed to top level cell coords, with
                            //  oNext field pointing at real odesc.
    const CDc **el_stack;   // array of pointers to parent odescs, first
                            //  element is list size, top down.
    char *el_message;       // Text of the error message.
    char *el_descr;         // Description of the error.
    Point_c el_pbad[5];     // Error region to highlight, in coords of
                            //  top level cell.
};

// Error report list, from tests.
//
struct DRCerrRet
{
    DRCerrRet(const DRCtestDesc*, int);

    DRCerrRet *filter();
    char *errmsg(const CDo*);
    const char *which_test(char*);

    void free()
        {
            DRCerrRet *er = this;
            while (er) {
                DRCerrRet *ex = er;
                er = er->next();
                delete ex;
            }
        }

    void set_next(DRCerrRet *er)  { er_next = er; }

    void set_pbad(unsigned int i, int x, int y)
        {
            if (i < 5)
                er_pbad[i].set(x, y);
        }

    void set_pbad(const BBox &BB)
        {
            BB.to_path(er_pbad);
        }

    void set_errtype(unsigned int et)
        {
            er_errtype = et;
        }

    DRCerrRet *next()           const { return (er_next); }
    const DRCtestDesc *rule()   const { return (er_rule); }
    const Point &pbad(unsigned int i)
                                const { return (er_pbad[i < 5 ? i : 4]); }
    unsigned int errtype()      const { return (er_errtype); }
    int vcount()                const { return (er_vcount); }


private:
    DRCerrRet *er_next;
    const DRCtestDesc *er_rule; // rule violated
    Point_c er_pbad[5];         // untransformed violation region
    unsigned int er_errtype;    // error code (one of TT_xxx)
    int er_vcount;              // edge/vertex count
};


// List element for edge condition, specifies one element of conjunction
// determining part of edge to apply test to.
//
struct DRCedgeCnd
{
    DRCedgeCnd(DRCwhereType w, const char *ex)
        {
            ec_next = 0;
            ec_where = w;
            ec_lspec.set_lname(ex);
        }

    void free()
        {
            DRCedgeCnd *e = this;
            while (e) {
                DRCedgeCnd *ex = e;
                e = e->ec_next;
                delete ex;
            }
        }

    bool setup();
    void print(sLstr*);

    DRCedgeCnd *next()                    { return (ec_next); }
    void setNext(DRCedgeCnd *e)           { ec_next = e; }

    DRCwhereType where()            const { return (ec_where); }

    sLspec *lspec()                       { return (&ec_lspec); }

private:
    DRCedgeCnd *ec_next;
    DRCwhereType ec_where;      // look outside/inside
    sLspec ec_lspec;            // what to look for
};


// List element for a test, which is applied to selected edge regions
// and corner areas.
//
struct DRCtestCnd
{
    DRCtestCnd(DRCwhereType w, int d, const char *ex)
        {
            tc_next = 0;
            tc_where = w;
            tc_dimension = d;
            tc_lspec.set_lname(ex);
        }

    void free()
        {
            DRCtestCnd *t = this;
            while (t) {
                DRCtestCnd *tx = t;
                t = t->tc_next;
                delete tx;
            }
        }

    char *init();
    void print(sLstr*);

    DRCtestCnd *next()                    { return (tc_next); }
    void setNext(DRCtestCnd *t)           { tc_next = t; }

    DRCwhereType where()            const { return (tc_where); }
    int dimension()                 const { return (tc_dimension); }

    sLspec *lspec()                       { return (&tc_lspec); }

private:
    DRCtestCnd *tc_next;
    DRCwhereType tc_where;      // test region
    int tc_dimension;           // test area width
    sLspec tc_lspec;            // what to look for
};


// Structure to hold the elements of a user-defined rule.
//
struct DRCtest
{
    DRCtest(char *n)
        {
            t_next = 0;
            t_name = n;
            t_argv = 0;
            t_edges = 0;
            t_eminstr = t_emaxstr = 0;
            t_edgemin = t_edgemax = 0;
            t_tests = 0;
            t_fail = 0;
            t_evaltree = 0;
            t_evalfunc = 0;
            t_variables = 0;
            t_edgeZlist = 0;
            t_edgecode = 0;
            t_argc = 0;
            t_cotstr = 0;
            t_cotwidth = 0;
        }

    ~DRCtest();

    bool substitute(const DRCtest*);
    char *init();
    void setvars(int, bool, const Zoid*);
    void print(sLstr*);
    bool is_edge_ok(const Zoid*, int);
    XIrt edgeTest(DRCtestDesc*, DRCedgeEval*, const Zlist*);
    XIrt cornerTest(DRCtestDesc*, DRCedgeEval*);
    XIrt cornerOverlapTest(DRCtestDesc*, DRCedgeEval*);

    char *argString() const
        {
            sLstr lstr;
            for (int i = 0; i < t_argc; i++) {
                if (lstr.length())
                    lstr.add_c(' ');
                lstr.add(t_argv[i]);
            }
            return (lstr.string_trim());
        }

    DRCtest *next()                 { return (t_next); }
    void setNext(DRCtest *t)        { t_next = t; }

    const char *name()              const { return (t_name); }

    int argc()                      const { return (t_argc); }

    const char *argv(int i) const
        {
            if (i >= 0 && i < t_argc)
                return (t_argv[i]);
            return (0);
        }

    void setArgs(int ac, char **av)
        {
            for (int i = 0; i < t_argc; i++)
                delete [] t_argv[i];
            delete [] t_argv;
            t_argc = ac;
            t_argv = av;
        }

    DRCedgeCnd *edges()             const { return (t_edges); }

    void setEdges(DRCedgeCnd *e) {
        t_edges->free();
        t_edges = e;
    }

    DRCtestCnd *tests()             const { return (t_tests); }

    void setTests(DRCtestCnd *t)
        {
            t_tests->free();
            t_tests = t;
        }

    const char *fail()              const { return (t_fail); }

    void setFail(const char *s)
        {
            char *t = lstring::copy(s);
            delete [] t_fail;
            t_fail = t;
        }

    void setEminStr(const char *s)
        {
            char *t = lstring::copy(s);
            delete [] t_eminstr;
            t_eminstr = t;
        }

    void setEmaxStr(const char *s)
        {
            char *t = lstring::copy(s);
            delete [] t_emaxstr;
            t_emaxstr = t;
        }

    int cotWidth()                  const { return (t_cotwidth); }
    const char *cotStr()            const { return (t_cotstr); }

    void setCotStr(const char *s)
        {
            char *t = lstring::copy(s);
            delete [] t_cotstr;
            t_cotstr = t;
        }

private:
    DRCtest *t_next;
    char *t_name;               // keyword designating the rule
    char **t_argv;              // list of parameters to rule
    DRCedgeCnd *t_edges;        // list of edge elements for conjunction
    char *t_eminstr;            // text for min edge
    char *t_emaxstr;            // text for max edge;
    int t_edgemin, t_edgemax;   // edge filter parameters
    DRCtestCnd *t_tests;        // list of tests to perform
    char *t_fail;               // text of fail condition expression
    ParseNode *t_evaltree;      // parse tree for fail condition ...
    SIfunc *t_evalfunc;         // ... or function block for fail processing
    siVariable *t_variables;    // variables used during fail evaluation
    Zlist *t_edgeZlist;         // list of edge regions
    int t_edgecode;             // cache edge orientation type
    int t_argc;                 // number of parameters in argv
    char *t_cotstr;             // corner overlap test string
    int t_cotwidth;             // corner overlap test dimension
};


typedef bool(*ErrFuncCb)(DRCtestDesc*, int, void*);

// Class for edge evaluation.
//
struct DRCedgeEval
{
    DRCedgeEval(PolyObj*);
    ~DRCedgeEval();

    // drc_rule.cc
    bool advance();
    void addAccumZlist(Zlist*);
    void initAng();
    bool wireCheck(int);

    const Point &p1()                   const { return (e_p1); }
    const Point &p2()                   const { return (e_p2); }
    const Point &p3()                   const { return (e_p3); }

    int passcnt()                       const { return (e_passcnt); }
    int edgecode()                      const { return (e_edgecode); }
    int ang()                           const { return (e_ang); }

    void set_edgecode(int c)                  { e_edgecode = c; }
    void set_elist(Zlist *el)                 { e_elist = el; }

    // The passcnt is sort of funny.  These functions return sensible
    // 0-based indices for vertices and sides.
    //
    int corner_indx()                   const { return (e_passcnt ?
                                                        e_passcnt - 1 : 0); }
    int edge_indx()                     const { return (e_passcnt - 1); }

    PolyObj *epoly()                    const { return (e_epoly); }
    bool cw()                           const { return (e_cw); }
    bool inside()                       const { return (!e_cw); }
    bool outside()                      const { return (e_cw); }

    int intReturn()                     const { return (e_int_return); }
    void setIntReturn(int i)                  { e_int_return = i; }

    Zlist *accumZlist()
        {
            Zlist *zl = e_accum_zlist;
            e_accum_zlist = 0;
            return (zl);
        }

    Zlist *elist()                      const { return (e_elist); }
    bool last_vtx()                     const { return (e_last_vtx); }
    bool check_wire_edge()              const { return (e_check_wire_edge); }

private:
    Point_c e_p1, e_p2, e_p3;   // Current edge points, p2-p3 is current edge
    const Point *e_vtx;         // Current vertex
    Zlist *e_elist;             // List of segments of edge (p2-p3) to test
    PolyObj *e_epoly;           // The figure containing the edge
    Zlist *e_accum_zlist;       // For "getZlist" pseudo-test
    int e_int_return;           // Integer test return
    int e_passcnt;              // Running count
    int e_edgecode;             // Indicates orientation, DRCeL etc.
    int e_ang;                  // Angle from previous edge
    bool e_cw;                  // Indicates winding direction
    bool e_last_vtx;            // True if final corner
    bool e_check_wire_edge;     // Internal state, wires allow skipping tests
    bool e_wire_checked;        // Internal state
};


// Edge codes for DRCtestDesc.
enum DRCedgeMode { EdgeDontCare, EdgeClear, EdgeCovered };

// Test callback for DRCtestDesc.
typedef XIrt(*DRCtestFunc)(Zoid*, DRCtestDesc*, DRCedgeEval*, bool*);


// Rule description and evaluation functions and state.
//
struct DRCtestDesc
{
    DRCtestDesc(DRCtype t, CDl *ld)
        {
            initTestDesc();
            td_objlayer = ld;
            td_type = t;
        }

    DRCtestDesc(DRCtestDesc *td, char *s, int dm, DRCtest *u)
        {
            initTestDesc();
            td_info[0] = s;
            td_objlayer = td->td_objlayer;
            td_user_rule = u;
            td_type = td->td_type;
            td_inhibit = td->td_inhibit;
            setDimen(dm);
        }

    DRCtestDesc(const char*, const char*, const char*, const char*, int,
        DRCedgeMode, DRCedgeMode, bool*);

    ~DRCtestDesc();

    void initTestDesc()
        {
            td_next                 = 0;
            td_info[0]              = 0;
            td_info[1]              = 0;
            td_info[2]              = 0;
            td_info[3]              = 0;
            td_origstring           = 0;
            td_objlayer             = 0;
            td_user_rule            = 0;
            td_testpoly             = 0;
            td_testfunc             = 0;

            td_u.area               = 0.0;
            td_spacing              = 0;

            td_type                 = drNoRule;
            td_where                = DRCoutside;
            td_edge_inside          = EdgeDontCare;
            td_edge_outside         = EdgeDontCare;

            td_result               = 0;
            td_paramset             = 0;
            td_inhibit              = 0;
            td_initialized          = false;

            td_edgrslt[0]           = 0;
            td_edgrslt[1]           = 0;
            td_edgrslt[2]           = 0;
            td_edgrslt[3]           = 0;
            td_has_target           = false;
            td_opptest              = false;
            td_no_area              = false;
            td_no_test_corners      = false;
            td_no_cot               = false;
            td_no_skip              = false;
            td_testdim              = 0;
            td_testdim2             = 0;
            td_values[0]            = 0;
            td_values[1]            = 0;
            td_values[2]            = 0;
            td_values[3]            = 0;
            td_cw1                  = 0;
            td_cw2                  = 0;
            td_fcw                  = 0;
            td_lcw                  = 0;
            td_fudge                = 0;
            td_ang_min              = 180;
        }

    bool initialize();
    const char *ruleName() const;
    char *regionString() const;
    void initEdgeTest();
    XIrt initEdgeList(DRCedgeEval*);
    XIrt getEdgeList(DRCedgeEval*, Zlist**, bool*);
    XIrt polyEdgesPreTest(DRCedgeEval*, ErrFuncCb = 0, void* = 0);
    XIrt polyEdgesTest(DRCedgeEval*, ErrFuncCb = 0, void* = 0);

    void edgeSetDimen(const DRCedgeEval*);
    bool edgeTestArea(const Point&, const Point&, const Zoid*, int,
        Zoid*, Point*);
    XIrt edgeTest(DRCedgeEval*, const Zlist*);
    bool cornerTestArea(const Point&, const Point&, const Point&,
        bool, Point*);
    XIrt cornerTest(DRCedgeEval*);
    XIrt cornerOverlapTest(DRCedgeEval*, int);

    void treeError();
    void seterr(const Zoid*, int);
    void seterr(const Point*, int);
    void seterr(const Point*, int, int);
    void seterr_pt(const Point*, int);

    // Callbacks, td_testfunc can point to one of these.  We use
    // static functions and a non-class pointer since 'this' is not
    // used in some of these.
    //
    static XIrt testEmpty(Zoid*, DRCtestDesc*, DRCedgeEval*, bool*);
    static XIrt testFull(Zoid*, DRCtestDesc*, DRCedgeEval*, bool*);
    static XIrt testNotFull(Zoid*, DRCtestDesc*, DRCedgeEval*, bool*);
    static XIrt testEdgeLen(Zoid*, DRCtestDesc*, DRCedgeEval*, bool*);
    static XIrt getEdgeLen(Zoid*, DRCtestDesc*, DRCedgeEval*, bool*);
    static XIrt getZlist(Zoid*, DRCtestDesc*, DRCedgeEval*, bool*);

    // drc_tech.cc
    const char *parse(const char*, const char*, DRCtest* = 0);
    void print(FILE*, sLstr*, bool, const CDl*, bool, bool) const;
    void printComment(FILE*, sLstr*) const;

    // drc_util.cc
    XIrt bloat(const CDo*, Zlist**, bool = false);
    static const char *ruleName(DRCtype);
    static DRCtype ruleType(const char*);
    static bool requiresTarget(DRCtype);
    static DRCtestDesc *findRule(CDl*, DRCtype, CDl*);

    DRCtestDesc *next()               { return (td_next); }
    void setNext(DRCtestDesc *n)      { td_next = n; }

    const char *info(unsigned int i) const
        {
            if (i < 4)
                return (td_info[i]);
            return (0);
        }

    void setInfo(unsigned int i, const char *s)
        {
            if (i < 4) {
                char *str = lstring::copy(s);
                delete [] td_info[i];
                td_info[i] = str;
            }
        }

    const char *origSpec()      const { return (td_origstring); }
    CDl *layer()                const { return (td_objlayer); }
    void setLayer(CDl *ld)            { td_objlayer = ld; }

    const DRCtest *userRule()   const { return (td_user_rule); }
    void setUserRule(DRCtest *t)      { td_user_rule = t; }

    bool matchUserRule(const char *n)
        {
            return (td_user_rule && !strcmp(td_user_rule->name(), n));
        }

    DRCtestDesc *testPoly(PolyObj *po, const CDl *ld, DRCerrCx &cx, XIrt *prt )
        {
            return ((this->*td_testpoly)(po, ld, cx, prt));
        }

    bool doCOT(int a)   const { return (a >= (360 - td_ang_min)); }
    bool doCWT(int a)   const { return (a <= td_ang_min); }
    bool doCST(int a)   const { return (a >= (360 - td_ang_min)); }

    void setSourceLayer(CDl *ld)    { td_source.set_ldesc(ld); }

    char *sourceString(bool nr = false) const
                                    { return (td_source.string(nr)); }

    // No need to check td_test.source()->ldesc, if a different source
    // layer was specified, it is ANDed with the objlayer so becomes a
    // tree.
    bool hasRegion()          const { return (td_source.tree() != 0); }

    ParseNode *sourceTree()         { return (td_source.tree()); }

    CDll *sourceLayers()            { return (td_source.findLayers()); }

    XIrt srcZlist(SIlexprCx *cx, Zlist **zret, bool clr)
        {
            return (td_source.getZlist(cx, zret, clr));
        }

    void rotateTarget(sLspec *lsbak, const sLspec *ls)
        {
            if (lsbak)
                *lsbak = td_target;
            if (ls)
                td_target = *ls;
        }

    char *targetString(bool nr = false) const
                                    { return (td_target.string(nr)); }

    CDl *targetLayer()
        {
            if (td_target.tree())
                return (0);
            if (td_target.ldesc())
                return (td_target.ldesc());
            if (td_target.lname())
                return (CDldb()->findLayer(td_target.lname(), Physical));
            return (0);
        }

    void setTargetLayer(CDl *ld)
        {
            td_target.set_ldesc(ld);
            td_target.set_lname(0);
            td_target.set_tree(0);
        }

    CDll *targetLayers()            { return (td_target.findLayers()); }

    char *insideString(bool nr = false) const
                                    { return (td_inside.string(nr)); }

    char *outsideString(bool nr = false) const
                                    { return (td_outside.string(nr)); }

    int dimen()                     const { return (td_u.dimen); }
    void setDimen(int d)                  { td_u.dimen = d; }
    double area()                   const { return (td_u.area); }
    void setArea(double a)                { td_u.area = a; }

    void setTestFunc(DRCtestFunc f)       { td_testfunc = f; }

    const sTspaceTable *spaceTab()  const { return (td_spacing); }
    void setSpaceTab(sTspaceTable *t)
        {
            delete [] td_spacing;
            td_spacing = t;
        }

    DRCtype type()                  const { return ((DRCtype)td_type); }
    DRCwhereType where()            const { return (
                                            (DRCwhereType)td_where); }
    void setWhere(DRCwhereType w)         { td_where = w; }

    DRCedgeMode edgeInside()        const { return (
                                            (DRCedgeMode)td_edge_inside); }
    DRCedgeMode edgeOutside()       const { return (
                                            (DRCedgeMode)td_edge_outside); }

    int result()                    const { return (td_result); }
    void setResult(int r)                 { td_result = r; }

    bool inhibited()                const { return (td_inhibit); }
    void setInhibited(bool b)             { td_inhibit = b; }

    bool oppTest()                  const { return (td_opptest); }
    bool noTestCorners()            const { return (td_no_test_corners); }
    void setNoCot(bool b)                 { td_no_cot = b; }
    bool noSkip()                   const { return (td_no_skip); }
    void setNoSkip(bool b)                { td_no_skip = b; }

    int testDim()                   const { return (td_testdim); }
    void setTestDim(int d)                { td_testdim = d; }
    int testDim2()                  const { return (td_testdim2); }

    int value(unsigned int i) const
        {
            if (i < 4)
                return (td_values[i]);
            return (0);
        }

    void setValue(unsigned int i, int v)
        {
            if (i < 4) {
                td_values[i] = v;
                td_paramset |= (1 << i);
            }
        }

    bool hasValue(unsigned int i) const
        {
            if (i < 4)
                return (td_paramset & (1 << i));
            return (false);
        }

    void initCw(int cw1, int cw2, bool first)
        {
            td_cw1 = cw1;
            td_cw2 = cw2;
            if (first)
                td_fcw = td_cw1;
        }

    void finalCw()                        { td_cw1 = td_fcw; td_cw2 = 0; }
    void lastCw()                         { td_lcw = td_cw2; }
    bool testCw()                   const { return (!td_lcw && !td_cw1); }
    int fudge()                     const { return (td_fudge); }

    const Point &getPbad(unsigned int i) const
        {
            return (td_pbad[i < 5 ? i : 4]);
        }

    bool hasTarget()   const
        { return (td_target.tree() || td_target.ldesc() || td_has_target); }
    bool hasInside()   const
        { return (td_inside.tree() || td_inside.ldesc()); }
    bool hasOutside()  const
        { return (td_outside.tree() || td_outside.ldesc()); }

private:
    // drc_rule.cc
    DRCtestDesc *testPoly_NOOP(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_FAIL(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_Overlap(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_IfOverlap(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_NoOverlap(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_AnyOverlap(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_PartOverlap(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_AnyNoOverlap(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_MinArea(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_MaxArea(PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestDesc *testPoly_EdgeTest(PolyObj*, const CDl*, DRCerrCx&, XIrt*);

    XIrt testOverlap(const Zlist*, bool*);
    XIrt testIfOverlap(const Zlist*, bool*);
    XIrt testNoOverlap(const Zlist*, bool*);
    XIrt testAnyOverlap(const Zlist*, bool*);
    XIrt testPartOverlap(const Zlist*, bool*);
    XIrt testAnyNoOverlap(const Zlist*, bool*);

    DRCerrRet *testPolyEdges(PolyObj*, XIrt*, bool = false);
    PolyList *expandRegions(PolyObj*);
    XIrt testPolyCovered(Point*, DRCedgeEval*, bool*);
    XIrt spaceTest(Zoid*, DRCedgeEval*, int*);

    // drc_tech.cc
    const char *parseConnected(const char*);
    const char *parseNoHoles(const char*);
    const char *parseXoverlap(const char*);
    const char *parseXarea(const char*);
    const char *parseMinEdgeLength(const char*);
    const char *parseMaxWidth(const char*);
    const char *parseMinWidth(const char*);
    const char *parseMinSpaceX(const char*);
    const char *parseMinSpaceFrom(const char*);
    const char *parseMinXoverlap(const char*);
    const char *parseUserDefinedRule(const char*, DRCtest*);
    bool parseRegion(const char**);
    bool parseInside(const char**);
    bool parseOutside(const char**);
    bool parseTarget(const char**);
    void parseComment(const char*);

    // Filter out rules that have nothing to do with the target layer.
    //
    bool skipThis(const CDl *ldtarget)
        {
            if (td_target.ldesc()) {
                if (td_target.ldesc() != ldtarget)
                    return (true);
            }
            else if (td_target.tree()) {
                if (!td_target.tree()->isLayerInTree(ldtarget))
                    return (true);
            }
            return (false);
        }

    DRCtestDesc *td_next;       // next rule
    char *td_info[4];           // user supplied info strings
    char *td_origstring;        // copied original specificaton string
    CDl *td_objlayer;           // layer of object being tested
    DRCtest *td_user_rule;      // used-defined test structure
    DRCtestDesc *(DRCtestDesc::*td_testpoly)
        (PolyObj*, const CDl*, DRCerrCx&, XIrt*);
    DRCtestFunc td_testfunc;    // test callback

    sLspec td_source;           // specification of regions to test
    sLspec td_target;           // referenced layer or expression
    sLspec td_inside;           // opt. inside edge condition
    sLspec td_outside;          // opt. outside edge condition
    union {
        double area;
        int dimen;
    } td_u;                     // test parameter.
    sTspaceTable *td_spacing;   // spacing table

    char td_type;               // test type code (DRCtype)
    char td_where;              // source test orientation (DRCwhereType)
    char td_edge_inside;        // target edge mode (DRCedgeMode)
    char td_edge_outside;       // target edge mode (DRCedgeMode)

    unsigned char td_result;    // error code (TT_xxx), 0 if passed
    char td_paramset;           // set bits indicate params used
    char td_inhibit;            // set to temporarily disable
    bool td_initialized;        // flag indicating initialization

    char td_edgrslt[4];         // for Opp test;
    bool td_has_target;         // a target spec is required
    bool td_opptest;            // do Opp test
    bool td_no_area;            // don't expand test zoids
    bool td_no_test_corners;    // skip the corner tests
    bool td_no_cot;             // skip corner overlap test
    bool td_no_skip;            // don't skip tests for wires and rects
    int td_testdim;             // actual dimension used
    int td_testdim2;            // second dimension used for Opp test
    int td_values[4];           // misc parameters for some tests
    int td_cw1;                 // width of edge seg from first vertex
    int td_cw2;                 // width of edge seg from second vertex
    int td_fcw;                 // cw1 of first edge
    int td_lcw;                 // cw2 of previous edge
    int td_fudge;               // fudge distance
    int td_ang_min;             // minimum angle for corner tests
    Point_c td_pbad[5];         // error region if test fails
};


// Maintainer for error lists.
//
struct DRCerrCx
{
    DRCerrCx()
        {
            ec_start = ec_end = 0;
            ec_flags = 0;
        }

    ~DRCerrCx()
        {
            ec_start->free();
        }

    void add(const DRCtestDesc *dd, int vc)
        {
            if (!ec_start)
                ec_start = ec_end = new DRCerrRet(dd, vc);
            else {
                ec_end->set_next(new DRCerrRet(dd, vc));
                ec_end = ec_end->next();
            }
            set_flag(dd->type());
        }

    void add(DRCerrRet *r)
        {
            if (r) {
                if (!ec_start) {
                    ec_start = ec_end = r;
                    set_flag(r->rule()->type());
                }
                else
                    ec_end->set_next(r);
                while (ec_end->next()) {
                    ec_end = ec_end->next();
                    set_flag(ec_end->rule()->type());
                }
            }
        }

    DRCerrRet *get_errs()
        {
            DRCerrRet *e = ec_start;
            ec_start = 0;
            return (e);
        }

    bool has_errs()             const { return (ec_start != 0); }
    void set_flag(int t)              { ec_flags |= (1 << t); }
    void unset_flag(int t)            { ec_flags &= ~(1 << t); }
    bool is_flag(int t)         const { return (ec_flags & (1 << t)); }

private:
    DRCerrRet *ec_start;
    DRCerrRet *ec_end;
    unsigned int ec_flags;
};

struct DRCjob
{
    DRCjob(const char *cn, unsigned int p, DRCjob *n)
        {
            j_next = n;
            j_cname = lstring::copy(cn);
            j_pid = p;
        }

    ~DRCjob()
        {
            delete [] j_cname;
        }

    const char *cellname()      const { return (j_cname); }
    unsigned int pid()          const { return (j_pid); }
    DRCjob *next()              const { return (j_next); }
    void set_next(DRCjob *n)          { j_next = n; }

private:
    DRCjob *j_next;
    const char *j_cname;
    unsigned int j_pid;
};


// Run initializing flags.
#define DRC_INIT_SKIP_CNT 0x1
#define DRC_INIT_PRGSIV   0x2

// Error reporting level:
//   One error per object.
//   One error of each type per object.
//   All errors.
enum DRClevelType { DRCone_err, DRCone_type, DRCall_errs };

// Enum for cDRC::UseLayerList and sDRC::UseRuleList.
enum DrcListType { DrcListNone, DrcListOnly, DrcListSkip };

inline class cDRC *DRC();

// The main class for DRC.
//
class cDRC : public cDrcIf
{
public:
    friend inline cDRC *DRC() { return (static_cast<cDRC*>(DrcIf())); }

    bool hasDRC() { return (true); }

    // drc.cc
    cDRC();
    bool actionHandler(WindowDesc*, int);                           // export
    int minDimension(CDl*);                                         // export
    void layerChangeCallback();                                     // export

    // drc_error.cc
    void showCurError(WindowDesc*, bool);                           // export
    void showError(WindowDesc*, bool, DRCerrList*);
    void eraseErrors(const BBox*);
    void eraseListError(const op_change_t*, bool);                  // export
    void clearCurError();                                           // export
    void fileMessage(const char*);                                  // export
    DRCerrList *findError(const BBox*);
    int dumpCurError(const char*);
    bool printFileHeader(FILE*, const char*, const BBox*, cCHD* = 0);
    void printFileEnd(FILE*, unsigned int, bool);
    static char *errFilename(const char*, int);

    // drc_eval.cc
    char *jobs();
    void runDRCregion(const BBox*);
    void runDRC(const BBox*, bool, cCHD* = 0, const char* = 0, bool = false);
    bool intrListTest(const op_change_t*, BBox*);                   // export
    XIrt batchTest(const BBox*, FILE*, sLstr*, BBox*, const char* = 0);
    XIrt gridBatchTest(const BBox*, FILE*);
    XIrt chdGridBatchTest(cCHD*, const char*, const BBox*, FILE*, bool);
    void batchListTest(const CDol*, FILE*, sLstr*, BBox*);
    XIrt layerRules(const CDl*, const BBox*, DRCerrRet**);
    XIrt objectRules(const CDo*, const BBox*, DRCerrRet**, CDl* = 0);
    int haloWidth();
#ifndef WIN32
    int forkToBackground();
#endif

    // drc_menu.cc
    MenuBox *createMenu();

    // drc_poly.c
    bool diskEval(int, int, const CDl*);                            // export
    bool donutEval(int, int, int, int, const CDl*);                 // export
    bool arcEval(int, int, int, int, double, double, const CDl*);   // export

    // drc_results.cc
    void viewErrsSequentiallyExec(CmdDesc *cmd);
    void cancelNext();                                              // export
    void windowDestroyed(int);                                      // export
    int errCb(int, const char*, int);
    void errReset(const char*, int);
    void setErrlist();
    void updateErrlist(drc_results::DRCresultParser*);
    bool errLayer(const char*, int);
    void dumpResultsFile();

    // drc_rule.cc
    void initRules();                                               // export
    void deleteRules(DRCtestDesc**);                                // export

    // drc_tech.cc
    void linkRule(DRCtestDesc*);
    DRCtestDesc *unlinkRule(DRCtestDesc*);
    bool addMaxWidth(CDl*, double, const char*);                    // export
    bool addMinWidth(CDl*, double, const char*);                    // export
    bool addMinDiagonalWidth(CDl*, double, const char*);            // export
    bool addMinSpacing(CDl*, CDl*, double, const char*);            // export
    bool addMinSameNetSpacing(CDl*, CDl*, double, const char*);     // export
    bool addMinDiagonalSpacing(CDl*, CDl*, double, const char*);    // export
    bool addMinSpaceTable(CDl*, CDl*, sTspaceTable*);               // export
    bool addMinArea(CDl*, double, const char*);                     // export
    bool addMinHoleArea(CDl*, double, const char*);                 // export
    bool addMinHoleWidth(CDl*, double, const char*);                // export
    bool addMinExtension(CDl*, CDl*, double, const char*);          // export
    bool addMinEnclosure(CDl*, CDl*, double, const char*);          // export
    bool addMinOppExtension(CDl*, CDl*, double, double,
        const char*);                                               // export
    TCret techParseLayer(CDl*);
    bool techParseUserRule(FILE*, const char** = 0);
    void techPrintLayer(FILE*, sLstr*, bool, const CDl*, bool,
        bool);
    void techPrintUserRules(FILE*);
    char *spacings(const char*);                                    // export
    char *orderedSpacings(const char*);                             // export
    char *minSpaceTables(const char*);                              // export

    // drc_user.cc
    XIrt userTest(int, bool*);
    XIrt userEmpty(int, bool*);
    XIrt userFull(int, bool*);
    XIrt userZlist(int, Zlist**);
    XIrt userEdgeLength(int, double*);

    // drc_util.c
    int outsideAngle(const Point*, const Point*, const Point*, bool);

    // funcs_drc.cc
    void loadScriptFuncs();

    // implemented in the graphics package
    void PopUpRules(GRobject, ShowMode);
    void PopUpDrcLimits(GRobject, ShowMode);
    void PopUpDrcRun(GRobject, ShowMode);
    void PopUpRuleEdit(GRobject, ShowMode, DRCtype, const char*,
        bool(*)(const char*, void*), void*, const DRCtestDesc*);

    //
    // access to private members
    //

    const char *layerList()   const { return (drc_layer_list); }
    void setLayerList(const char *ll)
        {
            char *s = lstring::copy(ll);
            delete [] drc_layer_list;
            drc_layer_list = s;
        }

    const char *ruleList()    const { return (drc_rule_list); }
    void setRuleList(const char *ll)
        {
            char *s = lstring::copy(ll);
            delete [] drc_rule_list;
            drc_rule_list = s;
        }

    FILE *errFilePtr()        const { return (drc_err_fp); }
    void setErrFilePtr(FILE *fp)    { drc_err_fp = fp; }
    bool isError()            const { return (drc_err_list != 0); }

    void removeJob(unsigned int p)
        {
            DRCjob *jp = 0;
            for (DRCjob *j = drc_job_list; j; j = j->next()) {
                if (j->pid() == p) {
                    if (jp)
                        jp->set_next(j->next());
                    else
                        drc_job_list = j->next();
                    delete j;
                    return;
                }
                jp = j;
            }
        }

    const Point *getObjPoints()
                              const { return (drc_obj_points); }
    int getObjNumPoints()     const { return (drc_obj_numpts); }
    int getObjCurVertex()     const { return (drc_obj_cur_vertex); }
    int getObjType()          const { return (drc_obj_type); }
    void setObjPoints(const Point *p, int n)
                                    { drc_obj_points = p; drc_obj_numpts = n; }
    void setObjCurVertex(int v)     { drc_obj_cur_vertex = v; }

    DrcListType useLayerList() const
                                { return ((DrcListType)drc_use_layer_list); }
    void setUseLayerList(DrcListType t)
                                    { drc_use_layer_list = t; }
    DrcListType useRuleList() const
                                { return ((DrcListType)drc_use_rule_list); }
    void setUseRuleList(DrcListType t)
                                    { drc_use_rule_list = t; }

    bool isShowZoids()        const { return (drc_show_zoids); }
    void setShowZoids(bool b)       { drc_show_zoids = b; }

    void setAbort(bool b)           { drc_abort = b; }

    int gridSize()            const { return (drc_grid_size); }
    void setGridSize(int g)         { drc_grid_size = g; }

    int getObjCount()         const { return (drc_obj_count); }
    int getErrCount()         const { return (drc_err_count); }

    unsigned int maxErrors()  const { return (drc_max_errors); }
    void setMaxErrors(int m)        { drc_max_errors = m; }
    unsigned intrMaxObjs()    const { return (drc_intr_max_objs); }
    void setIntrMaxObjs(int m)      { drc_intr_max_objs = m; }
    unsigned intrMaxTime()    const { return (drc_intr_max_time); }
    void setIntrMaxTime(int t)      { drc_intr_max_time = t; }
    unsigned intrMaxErrors()  const { return (drc_intr_max_errors); }
    void setIntrMaxErrors(int m)    { drc_intr_max_errors = m; }

    DRClevelType errorLevel() const { return ((DRClevelType)drc_err_level); }
    void setErrorLevel(DRClevelType l) { drc_err_level = l; }
    int fudgeVal()            const { return (drc_fudge); }
    void setFudgeVal(int f)
        {
            if (f >= DRC_FUDGE_MIN && f <= DRC_FUDGE_MAX)
                drc_fudge = f;
        }

    bool isInteractive()      const { return (drc_interactive); }   // export
    void setInteractive(bool b)     { drc_interactive = b; }        // export
    bool isIntrNoErrMsg()     const { return (drc_intr_no_errmsg);} // export
    void setIntrNoErrMsg(bool b)    { drc_intr_no_errmsg = b; }     // export
    bool isIntrSkipInst()     const { return (drc_intr_skip_inst); }
    void setIntrSkipInst(bool b)    { drc_intr_skip_inst = b; }

    bool ruleDisabled(DRCtype t)
                              const { return (drc_rule_disable[t]); }

    DRCtest *userTests()      const { return (drc_user_tests); }
    void setUserTests(DRCtest *t)   { drc_user_tests = t; }
    void setTestState(drc_user::TestState *t)
                                    { drc_test_state = t; }

private:
    // drc_eval.cc
    XIrt init_drc(const BBox*, Blist**, bool = false);
    bool handle_errors(DRCerrRet*, sPF*, const CDo*, BBox*, DRCerrList**,
        FILE*, sLstr*);
    int bloatmst(const CDl*, const CDl*);
    bool check_interrupt(bool);
    bool eval_instance(const CDc*, BBox*, const Blist*);
    void close_drc(Blist*);
    bool skip_layer(const CDl*);
    bool update_rule_disable();

    // drc_results.cc
    bool altShowError(WindowDesc*, bool);

    // drc_setif.cc
    void setupInterface();

    // drc_techif.cc
    void setupTech();

    // drc_txtcmds.cc
    void setupBangCmds();

    // drc_variables.cc
    void setupVariables();

    char *drc_layer_list;           // Layer filtering list.
    char *drc_rule_list;            // Rule filtering list.
    FILE *drc_err_fp;               // Pointer to current error file.
    DRCerrList *drc_err_list;       // Error list, built during evaluation.
    SymTab *drc_drv_tab;            // Derived layers we've used.
    DRCjob *drc_job_list;           // Running background jobs.
    unsigned long drc_check_time;   // Last time interrupts were tested.

    // Information for export during tests.
    const Point *drc_obj_points;
    int drc_obj_numpts;
    int drc_obj_cur_vertex;
    int drc_obj_type;

    char drc_use_layer_list;        // Layer filtering mode.
    char drc_use_rule_list;         // Rule filtering mode.

    bool drc_show_zoids;            // Show regions tested during evaluation.
    bool drc_doing_inter;           // Running interactive.
    bool drc_doing_grid;            // Using gridding.
    bool drc_with_chd;              // Obtaining geometry via a CHD.
    bool drc_abort;                 // Stop run now.

    unsigned int drc_grid_size;     // Area partitioning size.

    // These are set by the evaluation functions.
    unsigned int drc_obj_count;     // Number of objects to test.
    unsigned int drc_err_count;     // Number of errors found.
    unsigned int drc_num_checked;   // Number of objects tested.

    unsigned long drc_start_time;   // Time (msec) when DRC started.
    unsigned long drc_stop_time;    // Time (msec) when DRC terminated.

    unsigned int drc_max_errors;
    unsigned int drc_intr_max_objs;
    unsigned int drc_intr_max_time;
    unsigned int drc_intr_max_errors;
    char drc_err_level;
    char drc_fudge;
    bool drc_interactive;
    bool drc_intr_no_errmsg;
    bool drc_intr_skip_inst;

    bool drc_rule_disable[32];      // Rule disable flags (size must
                                    // accommodate number of rule tyles!)

    siVariable *drc_variables;      // Common context for layer variables.
    DRCtest *drc_user_tests;        // User-defined tests from tech file.
    drc_user::TestState *drc_test_state; // User rule execution context.
};

#endif

