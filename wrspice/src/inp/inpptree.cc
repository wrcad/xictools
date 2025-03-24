
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include <fenv.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <math.h>
#include "input.h"
#include "inpptree.h"
#include "circuit.h"
#include "simulator.h"
#include "variable.h"
#include "ttyio.h"
#include "verilog.h"
#include "spnumber/hash.h"
#include "miscutil/lstring.h"
#include "miscutil/errorrec.h"
#include "miscutil/random.h"
#include "ginterf/graphics.h"

#ifndef	M_2_SQRTPI
#define	M_2_SQRTPI  1.12837916709551257390  // 2/sqrt(pi)
#endif


// This exports Errs() to the whole application.
namespace { ErrRec errrec; }


// When parsing arguments to user defined functions, this struct is
// used to set up the formal/actual argument linkage, if we are
// linking the macro text into the calling tree.
//
struct sArgMap
{
    sArgMap(IFparseNode *a, const char *n, sArgMap *m)
        {
            args = a;
            names = n;
            next = m;
        }

    IFparseNode *find(const char*);

    IFparseNode *args;
    const char *names;
    sArgMap *next;
};


IFparseNode *
sArgMap::find(const char *name)
{
    if (!names)
        return (0);
    const char *n = names;
    IFparseNode *p = args;
    while (*n) {
        if (!p)
            return (0);
        if (!strcmp(n, name)) {
            if (p->type() == PT_COMMA)
                return (IFparseNode::copy(p->left()));
            return (IFparseNode::copy(p));
        }
        while (*n)
            n++;
        n++;
        p = p->right();
    }
    return (0);
}
// End of sArgMap functions


#define SPEC_TIME 0
#define SPEC_FREQ 1
#define SPEC_OMEG 2

namespace {
    // Special parameter tokens recognized.
    const char *specSigs[] = { "time", "freq", "omega", 0 };
}


// Constructor.
//
IFparseTree::IFparseTree(sCKT *ct, const char *xa)
{
    pt_ckt = ct;
    pt_line = 0;
    pt_tree = 0;
    pt_derivs = 0;
    pt_vars = 0;
    pt_xalias = lstring::copy(xa);
    pt_xtree = 0;
    pt_num_vars = 0;
    pt_num_xvars = 0;
    pt_argmap = 0;

    pt_pn_blocks = 0;
    pt_pn_used = 0;
    pt_pn_size = 0;

    pt_error = false;
    pt_macro = false;
}


// Destructor.
//
IFparseTree::~IFparseTree()
{
    delete [] pt_line;
    delete [] pt_xalias;
    delete [] pt_vars;
    delete [] pt_derivs;
    while (pt_pn_blocks) {
        pn_block *px = pt_pn_blocks;
        pt_pn_blocks = pt_pn_blocks->next;
        delete px;
    }
}


// Static function.
// Parse the expression in *line as far as possible, and return the
// parse tree obtained.  If there is an error, 0 is returned and an
// error message will be generated.  If pt_macro is set, the tree is
// not checked for unresolved references and the xalias is ignored.
//
// If ckt is passed null, no checking for unresolved references is
// done, and no circuit references can be resolved.
//
IFparseTree *
IFparseTree::getTree(const char **expstr, sCKT *ckt, const char *xalias,
    bool macro)
{
    if (!expstr)
        return (0);
    const char *t = *expstr;
    while (isspace(*t))
        t++;
    if (!*t)
        return (0);

    const char *ep = 0;
    char *tbf = 0;
    if (*t == '\'') {
        // If we find a single-quote, strip the quotes, and assume
        // that this contains the relevant part of the string.  Set
        // in ep the resumption point.

        t++;
        ep = strchr(t, '\'');
        if (ep) {
            int n = ep - t;
            tbf = new char[n+1];
            strncpy(tbf, t, n);
            tbf[n] = 0;
            ep++;
            t = tbf;
        }
    }

    // This function is reentered if there are user defined functions.
    // Save state.
    IFparseTree *tree = new IFparseTree(ckt, macro ? 0 : xalias);
    tree->pt_macro = macro;

    // Instantiate parser.
    PTelement elements[STACKSIZE];
    Parser P = Parser(elements, PRSR_NODEHACK | PRSR_USRSTR | PRSR_SOURCE);

    if (xalias && !macro) {
        // Create a tree for the "xalias".  This may increment numVars.
        // xalias is a a variable name to replace 'x' in the expression,
        // usually the scale variable.

        P.init(xalias, tree);
        tree->pt_xtree = P.parse();
        tree->pt_num_xvars = tree->pt_num_vars;
        // numXvars is one of
        //  0  for, e.g., x = time
        //  1  for, e.g., x = v(node)
        //  2  for, e.g., x = v(n+) - v(n-)
    }

    P.init(t, tree);
    IFparseNode *p = P.parse();
    if (!p || tree->pt_error || (ckt && !macro && !IFparseNode::check(p))) {
        delete tree;
        delete [] tbf;
        return (0);
    }
    p->collapse(&p);

    int nchars = P.residue() - t;
    if (ep)
        *expstr = ep;
    else
        *expstr = P.residue();
    char *tnew = new char[nchars+1];
    strncpy(tnew, t, nchars);
    tnew[nchars] = 0;
    delete [] tbf;
    tree->pt_line = tnew;
    tree->pt_tree = p;
    return (tree);
}


// Use an existing tree to parse a function, return the top parse
// node.  The nodes will be allocated from the tree allocator.
//
IFparseNode *
IFparseTree::treeParse(const char **expstr)
{
    if (!expstr)
        return (0);
    const char *t = *expstr;
    if (!t)
        return (0);
    while (isspace(*t))
        t++;
    if (!*t)
        return (0);

    const char *ep = 0;
    char *tbf = 0;
    if (*t == '\'') {
        // If we find a single-quote, strip the quotes, and assume
        // that this contains the relevant part of the string.  Set
        // in ep the resumption point.

        t++;
        ep = strchr(t, '\'');
        if (ep) {
            int n = ep - t;
            tbf = new char[n+1];
            strncpy(tbf, t, n);
            tbf[n] = 0;
            ep++;
            t = tbf;
        }
    }

    // Instantiate parser.
    PTelement elements[STACKSIZE];
    Parser P = Parser(elements, PRSR_NODEHACK | PRSR_USRSTR | PRSR_SOURCE);
    P.init(t, this);

    IFparseNode *p = P.parse();
    if (!p || pt_error || (pt_ckt && !pt_macro && !IFparseNode::check(p))) {
        pt_error = false;
        delete [] tbf;
        return (0);
    }
    p->collapse(&p);

    if (ep)
        *expstr = ep;
    else
        *expstr = P.residue();
    delete [] tbf;

    return (p);
}


bool
IFparseTree::differentiate()
{
    bool ok = true;
    if (pt_ckt && pt_num_vars && !pt_derivs) {

        pt_derivs = new IFparseNode*[pt_num_vars];
        for (int i = 0; i < pt_num_vars; i++) {
            IFparseNode *pd = differentiate(pt_tree, i);
            if (pd) {
                pd->collapse(&pd);
                pt_derivs[i] = IFparseNode::copy(pd, true);
            }
            else {
                pt_derivs[i] = p_mkcon(0.0);
                sLstr lstr;
                varName(i, lstr);
                Errs()->add_error(
                    "differentiation for variable %s failed, setting "
                    "derivative to 0.", lstr.string());
                ok = false;
            }
        }
    }
    return (ok);
}


bool
IFparseTree::isConst()
{
    return (IFparseNode::is_const(pt_tree));
}


namespace {
    inline int check_fpe(bool noerrret)
    {
        int err = OK;
#ifdef HAVE_FENV_H
        if (!noerrret && Sp.GetFPEmode() == FPEcheck) {
            if (fetestexcept(FE_DIVBYZERO))
                err = E_MATHDBZ;
            else if (fetestexcept(FE_OVERFLOW))
                err = E_MATHOVF;
            // Ignore underflow, really shouldn't be a problem, and
            // these are generated rather frequently.
            // else if (fetestexcept(FE_UNDERFLOW))
            //     err = E_MATHUNF;
            else if (fetestexcept(FE_INVALID))
                err = E_MATHINV;
        }
        feclearexcept(FE_ALL_EXCEPT);
        (void)noerrret;
#endif
        return (err);
    }
}


// Evaluate the tree.  If docomma is given, if the top of the tree is
// a comma, result will contain the 2 values, and *docomma will be
// incremented twice.  Otherwise, this is a syntax error.
//
int
IFparseTree::eval(double *result, double *vals, double *dvs, int *docomma)
{
    Errs()->init_error();
    if (!pt_tree)
        *result = 0.0;
    else {
        if (docomma && pt_tree->p_type == PT_COMMA) {
            int err = (pt_tree->p_left->*pt_tree->p_left->p_evfunc)(result,
                vals, 0);
            if (err != OK) {
                char *nstr = pt_tree->p_left->get_string(96);
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "Argument evaluation failed\n  node:  %s\n%s\n",
                    nstr, Errs()->get_error());
                delete [] nstr;
                return (err);
            }
            err = (pt_tree->p_right->*pt_tree->p_right->p_evfunc)(result+1,
                vals, 0);
            if (err != OK) {
                char *nstr = pt_tree->p_right->get_string(96);
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "Argument evaluation failed\n  node:  %s\n%s\n",
                    nstr, Errs()->get_error());
                delete [] nstr;
                return (err);
            }
            (*docomma) += 2;
        }
        else {
            check_fpe(true);
            int err = (pt_tree->*pt_tree->p_evfunc)(result, vals, 0);
            if (err != OK) {
                char *nstr = pt_tree->get_string(96);
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "expression evaluation failed\n  node:  %s\n%s\n",
                    nstr, Errs()->get_error());
                delete [] nstr;
                return (err);
            }
            if (check_fpe(false)) {
                sLstr lstr;
                lstr.add(
        "expression evaluation caused floating-point exception\n  node:  ");
                char *nstr = pt_tree->get_string(96);
                lstr.add(nstr);
                delete [] nstr;
                lstr.add_c('\n');
                if (pt_num_vars > 0) {
                    lstr.add("  vars:  ");
                    for (int i = 0; i < pt_num_vars; i++) {
                        varName(i, lstr);
                        lstr.add_c('=');
                        lstr.add_g(vals[i]);
                        lstr.add_c(' ');
                    }
                }
                GRpkg::self()->ErrPrintf(ET_WARN, "%s\n", lstr.string());
            }
        }
        if (dvs) {
            if (!pt_derivs)
                differentiate();
            for (int i = 0; i < pt_num_vars; i++) {
                check_fpe(true);
                int err = (pt_derivs[i]->*pt_derivs[i]->p_evfunc)(&dvs[i],
                    vals, 0);
                if (err != OK) {
                    char *nstr = pt_tree->get_string(96);
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                "derivative evaluation for %s failed\n  node:  %s\n%s\n",
                        nstr, Errs()->get_error());
                    delete [] nstr;
                    return (err);
                }
                if (check_fpe(false)) {
                    sLstr lstr;
                    lstr.add("derivative evaluation for ");
                    varName(i, lstr);
                    lstr.add(" caused floating-point exception\n  node:  ");
                    char *nstr = pt_tree->get_string(96);
                    lstr.add(nstr);
                    delete [] nstr;
                    GRpkg::self()->ErrPrintf(ET_ERROR, "%s\n", lstr.string());
                }
            }
        }
    }
    return (OK);
}


void
IFparseTree::print(const char *str)
{
    TTY.printf("%s\n\t", str);
    sLstr lstr;
    pt_tree->print_string(lstr);
    TTY.printf("%s\n", lstr.string());
    for (int i = 0; i < pt_num_vars; i++) {
        lstr.clear();
        lstr.add("d/d ");
        varName(i, lstr);
        lstr.add(" : ");
        pt_derivs[i]->print_string(lstr);
        TTY.printf("%s\n", lstr.string());
    }
}


void
IFparseTree::varName(int i, sLstr &lstr)
{
    if (i >= 0 && i < pt_num_vars) {
        if ((pt_vars[i].type & IF_VARTYPES) == IF_NODE) {
            IFnode n = pt_vars[i].v.nValue;
            lstr.add("v(");
            lstr.add((const char*)n->name());
            lstr.add_c(')');
            return;
        }
        if ((pt_vars[i].type & IF_VARTYPES) == IF_INSTANCE) {
            lstr.add((const char*)pt_vars[i].v.uValue);
            lstr.add("#branch");
            return;
        }
    }
    lstr.add("(?)");
}


// Initialize the "tran" functions, called just before transient
// analysis with step and finaltime nonzero, or just before other
// analyses with these set to 0.0.  In the latter case, functions
// other than PWL will return initial values only.  In the former
// case, we set the breakpoints and the functions will be fully
// enabled.
//
int
IFparseTree::initTranFuncs(double step, double finaltime)
{
    for (int i = 0; i < pt_num_vars; i++) {
        if (pt_derivs && pt_derivs[i])
            pt_derivs[i]->p_init_node(step, finaltime);
    }
    if (pt_tree)
        pt_tree->p_init_node(step, finaltime);
    return (OK);
}


double
IFparseTree::timeLim(double lim)
{
    return (pt_tree ? pt_tree->time_limit(pt_ckt, lim) : lim);
}


// This method takes the partial derivative of the parse tree with respect
// to the i'th variable.  We try to do optimizations like getting rid of
// 0-valued terms.  if varnum < 0, differentiate wrt "x".
//  numXvars == 0: d/dt time = 1, d/dt vars = 0
//  numXvars == 1: d/dt time = 0, d/dt vars[0] = 1
//  numXvars == 2: d/dt time = 0, d/dt vars[0] = 1
//
IFparseNode *
IFparseTree::differentiate(IFparseNode *p, int varnum)
{
    IFparseNode *arg1 = 0, *arg2, *newp;
    if (!p)
        return (0);

    switch (p->p_type) {
    case PT_CONSTANT:
        newp = p_mkcon(0.0);
        break;

    case PT_PARAM:
        if (varnum < 0 && pt_num_xvars == 0 && p->p_valindx == SPEC_TIME)
            newp = p_mkcon(1.0);
        else
            newp = p_mkcon(0.0);
        break;

    case PT_TFUNC:
        newp = p_mktrand(p, varnum);
        if (!newp)
            newp = p_mkcon(0.0);
        break;

    case PT_VAR:
        // Is this the variable we're differentiating wrt?
        if (p->p_valindx == varnum ||
                (varnum < 0 && pt_num_xvars && p->p_valindx == 0))
            newp = p_mkcon(1.0);
        else
            newp = p_mkcon(0.0);
        break;

    case PT_PLUS:
    case PT_MINUS:
        arg1 = differentiate(p->p_left, varnum);
        arg2 = differentiate(p->p_right, varnum);
        if (arg1->p_type == PT_CONSTANT && arg1->v.constant == 0) {
            if (p->p_type == PT_PLUS)
                newp = arg2;
            else
                newp = p_mkf(PTF_UMINUS, arg2);
        }
        else if (arg2->p_type == PT_CONSTANT && arg2->v.constant == 0)
            newp = arg1;
        else
            newp = p_mkb(p->p_type, arg1, arg2);
        break;

    case PT_TIMES:
        // d(a * b) = d(a) * b + d(b) * a
        arg1 = differentiate(p->p_left, varnum);
        arg2 = differentiate(p->p_right, varnum);
        if (arg1->p_type == PT_CONSTANT && arg1->v.constant == 0)
            newp = p_mkb(PT_TIMES, p->p_left, arg2);
        else if (arg2->p_type == PT_CONSTANT && arg2->v.constant == 0)
            newp = p_mkb(PT_TIMES, arg1, p->p_right);
        else
            newp = p_mkb(PT_PLUS, p_mkb(PT_TIMES, arg1, p->p_right),
                    p_mkb(PT_TIMES, p->p_left, arg2));
        break;

    case PT_DIVIDE:
        // d(a / b) = (d(a) * b - d(b) * a) / b^2 
        arg1 = differentiate(p->p_left, varnum);
        arg2 = differentiate(p->p_right, varnum);
        if (arg1->p_type == PT_CONSTANT && arg1->v.constant == 0)
            newp = p_mkb(PT_DIVIDE, p_mkf(PTF_UMINUS,
                p_mkb(PT_TIMES, p->p_left, arg2)),
                p_mkb(PT_POWER, p->p_right, p_mkcon(2.0)));
        else if (arg2->p_type == PT_CONSTANT && arg2->v.constant == 0)
            newp = p_mkb(PT_DIVIDE, arg1, p->p_right);
        else
            newp = p_mkb(PT_DIVIDE, p_mkb(PT_MINUS, p_mkb(PT_TIMES, arg1,
                p->p_right), p_mkb(PT_TIMES, p->p_left, arg2)),
                p_mkb(PT_POWER, p->p_right, p_mkcon(2.0)));
        break;

    case PT_POWER:
        // Two cases... If the power is a constant then we're cool.
        // Otherwise we have to be tricky.
        if (p->p_right->p_type == PT_CONSTANT) {
            arg1 = differentiate(p->p_left, varnum);
            newp = p_mkb(PT_TIMES, p_mkb(PT_TIMES,
                    p_mkcon(p->p_right->v.constant),
                    p_mkb(PT_POWER, p->p_left,
                    p_mkcon(p->p_right->v.constant - 1))),
                    arg1);
        }
        else {
            // This is complicated.  f(x) ^ g(x) -> exp(g(x) * ln(f(x)))
            // d/dx exp(g*ln(f)) = exp(g*ln(f)) * d/dx [g*ln(f)]
            // = exp(g*ln(f)) * [g'*ln(f) + g*(f'/f)]
            //
            arg1 = differentiate(p->p_left, varnum);
            arg2 = differentiate(p->p_right, varnum);
            newp = p_mkb(PT_TIMES,
                        p_mkf(PTF_EXP, p_mkb(PT_TIMES,
                            p->p_right, p_mkf(PTF_LN, p->p_left))),
                    p_mkb(PT_PLUS,
                        p_mkb(PT_TIMES, p->p_right,
                            p_mkb(PT_DIVIDE, arg1, p->p_left)),
                        p_mkb(PT_TIMES, arg2,
                            p_mkf(PTF_LN, p->p_left))));
        }
        break;

    case PT_COMMA:
        // Shouldn't really see this, represents a complex quantity
        // in a real expression.  Ignore the imaginary part
        newp = differentiate(p->p_left, varnum);
        break;

#ifdef NEWOPS
    case PT_EQ:
    case PT_GT:
    case PT_LT:
    case PT_GE:
    case PT_LE:
    case PT_NE:
    case PT_AND:
    case PT_OR:
    case PT_NOT:
        newp = p_mkcon(0.0);
        break;
#endif

    case PT_FUNCTION:
        if (p->p_left->p_type == PT_COMMA) {
            // case for jn, yn. pow
            arg2 = differentiate(p->p_left->p_right, varnum);
            if (p->p_valindx == PTF_JN) {
                //  (jn-1(u) - jn+1(u))/2
                if (arg2->p_type == PT_CONSTANT && arg2->v.constant == 0)
                    newp = arg2;
                else {
                    arg1 = p_mkb(PT_TIMES, p_mkcon(0.5), p_mkb(PT_MINUS,
                        p_mkf(PTF_JN, p_mkb(PT_COMMA,
                            p_mkcon(p->p_left->p_left->v.constant-1),
                            p->p_left->p_right)),
                        p_mkf(PTF_JN, p_mkb(PT_COMMA,
                            p_mkcon(p->p_left->p_left->v.constant+1),
                            p->p_left->p_right))));
                    newp = p_mkb(PT_TIMES, arg1, arg2);
                }
            }
            else if (p->p_valindx == PTF_YN) {
                //  (yn-1(u) - yn+1(u))/2
                if (arg2->p_type == PT_CONSTANT && arg2->v.constant == 0)
                    newp = arg2;
                else {
                    arg1 = p_mkb(PT_TIMES, p_mkcon(0.5), p_mkb(PT_MINUS,
                        p_mkf(PTF_YN, p_mkb(PT_COMMA,
                            p_mkcon(p->p_left->p_left->v.constant-1),
                            p->p_left->p_right)),
                        p_mkf(PTF_YN, p_mkb(PT_COMMA,
                            p_mkcon(p->p_left->p_left->v.constant+1),
                            p->p_left->p_right))));
                    newp = p_mkb(PT_TIMES, arg1, arg2);
                }
            }
            else if (p->p_valindx == PTF_POW) {
                IFparseNode *px = p->p_left->p_left;
                IFparseNode *py = p->p_left->p_right;
                // Two cases...  If the power is a constant then
                // we're cool.  Otherwise we have to be tricky.

                if (py->p_type == PT_CONSTANT) {
                    arg1 = differentiate(px, varnum);
                    newp = p_mkb(PT_TIMES,
                        p_mkb(PT_TIMES, p_mkcon(py->v.constant),
                        p_mkb(PT_POWER, px, p_mkcon(py->v.constant - 1))),
                        arg1);
                }
                else {
                    // This is complicated.
                    // f(x) ^ g(x) -> exp(g(x) * ln(f(x)))
                    // d/dx exp(g*ln(f)) = exp(g*ln(f)) * d/dx [g*ln(f)]
                    // = exp(g*ln(f)) * [g'*ln(f) + g*(f'/f)]
                    //
                    arg1 = differentiate(px, varnum);
                    arg2 = differentiate(py, varnum);
                    newp = p_mkb(PT_TIMES, p_mkf(PTF_EXP, p_mkb(PT_TIMES,
                        py, p_mkf(PTF_LN, px))),
                        p_mkb(PT_PLUS, p_mkb(PT_TIMES, py,
                        p_mkb(PT_DIVIDE, arg1, px)),
                        p_mkb(PT_TIMES, arg2,
                        p_mkf(PTF_LN, px))));
                }
            }
            else {
                // Error: only the functions above take more than one
                // argument.
                Errs()->add_error("wrong arg count for function index %d",
                    p->p_valindx);
                return (0);
            }
        }
        else {
            arg2 = differentiate(p->p_left, varnum);
            if (arg2->p_type == PT_CONSTANT && arg2->v.constant == 0) {
                newp = arg2;
                break;
            }

            switch (p->p_valindx) {
            case PTF_ABS:  // sgn(u)
                arg1 = p_mkf(PTF_SGN, p->p_left);
                break;

            case PTF_SGN:  // u -> a hack
                arg1 = p->p_left;
                break;

            case PTF_ACOS:  // - 1 / sqrt(1 - u^2)
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(-1.0), p_mkf(PTF_SQRT,
                    p_mkb(PT_MINUS, p_mkcon(1.0), p_mkb(PT_POWER, p->p_left,
                    p_mkcon(2.0)))));
                break;

            case PTF_ACOSH: // 1 / sqrt(u^2 - 1)
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p_mkf(PTF_SQRT,
                    p_mkb(PT_MINUS, p_mkb(PT_POWER, p->p_left, p_mkcon(2.0)),
                    p_mkcon(1.0))));

                break;

            case PTF_ASIN:  // 1 / sqrt(1 - u^2)
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p_mkf(PTF_SQRT,
                    p_mkb(PT_MINUS, p_mkcon(1.0), p_mkb(PT_POWER, p->p_left,
                    p_mkcon(2.0)))));
                break;

            case PTF_ASINH: // 1 / sqrt(u^2 + 1)
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p_mkf(PTF_SQRT,
                    p_mkb(PT_PLUS, p_mkb(PT_POWER, p->p_left, p_mkcon(2.0)),
                    p_mkcon(1.0))));
                break;

            case PTF_ATAN:  // 1 / (1 + u^2)
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p_mkb(PT_PLUS,
                    p_mkb(PT_POWER, p->p_left, p_mkcon(2.0)), p_mkcon(1.0)));
                break;

            case PTF_ATANH: // 1 / (1 - u^2)
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p_mkb(PT_MINUS,
                    p_mkcon(1.0), p_mkb(PT_POWER, p->p_left, p_mkcon(2.0))));
                break;

            case PTF_CBRT:  // 1/3 * u^-2/3)
                arg1 = p_mkb(PT_TIMES, p_mkcon(1.0/3), p_mkb(PT_POWER,
                    p->p_left, p_mkcon(-2.0/3)));
                break;

            case PTF_COS:   // - sin(u)
                arg1 = p_mkf(PTF_UMINUS, p_mkf(PTF_SIN, p->p_left));
                break;

            case PTF_COSH:  // sinh(u)
                arg1 = p_mkf(PTF_SINH, p->p_left);
                break;

            case PTF_ERF:   // 2/sqrt(pi) * exp(-u^2)
                arg1 = p_mkb(PT_TIMES, p_mkcon(M_2_SQRTPI), p_mkf(PTF_EXP,
                    p_mkf(PTF_UMINUS, p_mkb(PT_POWER, p->p_left, p_mkcon(2.0)))));
                break;

            case PTF_ERFC:  // -2/sqrt(pi) * exp(-u^2)
                arg1 = p_mkb(PT_TIMES, p_mkcon(-M_2_SQRTPI), p_mkf(PTF_EXP,
                    p_mkf(PTF_UMINUS, p_mkb(PT_POWER, p->p_left, p_mkcon(2.0)))));
                break;

            case PTF_EXP:   // exp(u)
                arg1 = p_mkf(PTF_EXP, p->p_left);
                break;

            case PTF_J0:    // -j1(u)
                arg1 = p_mkf(PTF_UMINUS, p_mkf(PTF_J1, p->p_left));
                break;

            case PTF_J1:    //  (j0(u) - j2(u))/2
                arg1 = p_mkb(PT_TIMES, p_mkcon(0.5), p_mkb(PT_MINUS,
                    p_mkf(PTF_J0, p->p_left), p_mkf(PTF_JN, p_mkb(PT_COMMA,
                    p_mkcon(2.0), p->p_left))));
                break;

            case PTF_LN:    // 1 / u
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p->p_left);
                break;

            case PTF_LOG10:   // log(e) / u
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(M_LOG10E), p->p_left);
                break;

            case PTF_SIN:   // cos(u)
                arg1 = p_mkf(PTF_COS, p->p_left);
                break;

            case PTF_SINH:  // cosh(u)
                arg1 = p_mkf(PTF_COSH, p->p_left);
                break;

            case PTF_SQRT:  // 1 / (2 * sqrt(u))
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p_mkb(PT_TIMES,
                    p_mkcon(2.0), p_mkf(PTF_SQRT, p->p_left)));
                break;

            case PTF_TAN:   // 1 / (cos(u) ^ 2)
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p_mkb(PT_POWER,
                    p_mkf(PTF_COS, p->p_left), p_mkcon(2.0)));
                break;

            case PTF_TANH:  // 1 / (cosh(u) ^ 2)
                arg1 = p_mkb(PT_DIVIDE, p_mkcon(1.0), p_mkb(PT_POWER,
                    p_mkf(PTF_COSH, p->p_left), p_mkcon(2.0)));
                break;

            case PTF_UMINUS: // - 1 ; like a constant (was 0 !)
                arg1 = p_mkcon(-1.0);
                break;

            case PTF_Y0:    //  -y1(u)
                arg1 = p_mkf(PTF_UMINUS, p_mkf(PTF_Y1, p->p_left));
                break;

            case PTF_Y1:    //  (y0(u) - y2(u))/2
                arg1 = p_mkb(PT_TIMES, p_mkcon(0.5), p_mkb(PT_MINUS,
                p_mkf(PTF_Y0, p->p_left), p_mkf(PTF_YN, p_mkb(PT_COMMA,
                p_mkcon(2.0), p->p_left))));
                break;

            default:
                Errs()->add_error("bad function index %d", p->p_valindx);
                return (0);
            }
            newp = p_mkb(PT_TIMES, arg1, arg2);
        }
        break;

    case PT_MACRO:
        {
            IFmacro *macro = p->v.macro;
            IFmacroDeriv *md = new IFmacroDeriv(macro);

            IFparseNode *a = p->p_left, *r;
            for (int i = 0; i < macro->numargs(); i++) {
                if (a->p_type == PT_COMMA) {
                    r = a->p_left;
                    a = a->p_right;
                }
                else {
                    r = a;
                    a = 0;
                }
                if (!r)
                    break;
                IFparseNode *ad = IFparseNode::copy(differentiate(r, varnum),
                    true);
                if (!ad) {
                    Errs()->add_error(
                        "macro %s argument %d differentiation failed",
                        p->p_valname, i+1);
                    delete md;
                    return (0);
                }
                md->md_argdrvs[i] = ad;
            }
            newp = newNode();
            newp->p_type = PT_MACRO_DERIV;
            newp->p_left = p->p_left;
            newp->p_evfunc = &IFparseNode::p_macro_deriv;
            newp->v.macro_deriv = md;
        }
        break;

    case PT_MACROARG:
        if (p->p_valindx == varnum)
            newp = p_mkcon(1.0);
        else
            newp = p_mkcon(0.0);
        break;

    case PT_MACRO_DERIV:
        // fall through.  Write this someday to handle second and larger
        // derivitives with macros.

    default:
        Errs()->add_error("bad node type %d", p->p_type);
        return (0);
    }
    return (newp);
}


// Memory management.
//
IFparseTree::pn_block::pn_block(int sz, pn_block *n) : next(n),
    block(new IFparseNode[sz]), size(sz) { }
IFparseTree::pn_block::~pn_block() { delete [] block; }

// Call to obtain a new IFparseNode.  Nodes are cleared when the tree
// is deleted, and should never be created/deleted otherwise.
// 
IFparseNode *
IFparseTree::newNode()
{
    if (pt_pn_used == pt_pn_size) {
        if (pt_pn_size == 0) {
            pt_pn_size = 8;
            pt_pn_blocks = new pn_block(pt_pn_size, pt_pn_blocks);
        }
        else if (pt_pn_size == 256)
            pt_pn_blocks = new pn_block(pt_pn_size, pt_pn_blocks);
        else {
            pt_pn_size += pt_pn_size;
            pt_pn_blocks = new pn_block(pt_pn_size, pt_pn_blocks);
        }
        pt_pn_used = 0;
    }
    IFparseNode *p = &pt_pn_blocks->block[pt_pn_used];
    p->p_tree = this;
    pt_pn_used++;
    return (p);
}


IFparseNode *
IFparseTree::newNnode(double d)
{
    IFparseNode *p = newNode();
    p->p_type = PT_CONSTANT;
    p->v.constant = d;
    p->p_evfunc = &IFparseNode::p_const;
    return (p);
}


IFparseNode *
IFparseTree::newUnode(int token, IFparseNode *arg)
{
    if (token == TT_UMINUS) {
        IFparseNode *p = newNode();
        p->p_type = PT_FUNCTION;
        p->p_left = arg;
        p->p_valname = "-";
        p->p_valindx = PTF_UMINUS;
        p->p_func = &IFparseNode::PTuminus;
        p->p_evfunc = &IFparseNode::p_fcn;
        return (p);
    }
    return (0);
}


namespace {
    // These are operators that the parser might recognize and return
    // a node for, but are not handled.

    struct _dummy_ { int number; const char *name; } bad_ops[] = { 
        { TT_MOD,       "%"},
        { TT_COLON,     ":"},
        { TT_COND,      "?"},
        { TT_EQ,        "EQ"},
        { TT_GT,        "GT"},
        { TT_LT,        "LT"},
        { TT_GE,        "GE"},
        { TT_LE,        "LE"},
        { TT_NE,        "NE"},
        { TT_AND,       "AND"},
        { TT_OR,        "OR"},
        { TT_NOT,       "NOT"},
        { TT_INDX,      "INDX"},
        { TT_RANGE,     "RANGE"},
        { 0,            0}
    };
}

IFparseNode *
IFparseTree::newBnode(int token, IFparseNode *arg1, IFparseNode *arg2)
{
    int i;
    for (i = 0; IFparseNode::PTops[i].name; i++) {
        if ((int)IFparseNode::PTops[i].number == (int)token)
            break;
    }
    if (!IFparseNode::PTops[i].name) {
        for (i = 0; bad_ops[i].name; i++) {
            if (bad_ops[i].number == (int)token)
                break;
        }
        Errs()->add_error(
            "invalid operator number %d (\"%s\") in input", token,
            bad_ops[i].name ? bad_ops[i].name : "unknown");
        return (0);
    }

    IFparseNode *p = newNode();
    p->p_type = (PTtype)token;
    p->p_valname = IFparseNode::PTops[i].name;
    p->p_func = IFparseNode::PTops[i].funcptr;
    p->p_evfunc = &IFparseNode::p_op;
    p->p_left = arg1;
    p->p_right = arg2;
    return (p);
}


IFparseNode *
IFparseTree::newSnode(const char *string)
{
    if (!string)
        return (0);
    if (*string == '\\') {
        // Suppress any matching, for node names.
        IFparseNode *p = newNode();
        p->p_type = PT_PLACEHOLDER;
        p->p_valname = lstring::copy(string + 1);
        return (p);
    }

    if (lstring::cieq(string, "x")) {
        // This is the "xalias" variable (if any).
        if (xtree())
            return (IFparseNode::copy(xtree()));
    }

    if (pt_argmap) {
        // Reference to a macro argument.
        IFparseNode *p = pt_argmap->find(string);
        if (p)
            return (p);
    }

    IFparseNode *p = newNode();

    // First check it is really a tran function.
    int error;
    if (IFparseNode::parse_if_tranfunc(p, string, &error)) {
        if (error)
            pt_error = true;
        return (p);
    }

    // Next see if it's a special parameter.
    if (*string == '@') {
        p->p_valindx = -1;
        p->p_type = PT_PARAM;
        p->p_evfunc = &IFparseNode::p_parm;
        p->p_valname = lstring::copy(string);
        return (p);
    }
    
    // Is it in the specSigs list?
    for (int i = 0; specSigs[i]; i++) {
        if (!strcmp(specSigs[i], string)) {
            p->p_valindx = i;
            p->p_type = PT_PARAM;
            p->p_evfunc = &IFparseNode::p_parm;
            return (p);
        }
    }

    // Is it a named constant?
    for (sConstant *c = Sp.Constants(); c->name; c++) {
        if (lstring::cieq(c->name, string)) {
            p->p_type = PT_CONSTANT;
            p->v.constant = c->value;
            p->p_evfunc = &IFparseNode::p_const;
            return (p);
        }
    }

    // Is it a Verilog output?
    if (ckt() && ckt()->CKTvblk) {
        char *tstr = new char[strlen(string) + 2];
        strcpy(tstr, string);

        // Hide parameters behind a single null, the string is terminated
        // with double null.
        char *a = strchr(tstr, '[');
        if (a) {
            char *b = strchr(a, ']');
            *a++ = 0;
            if (b) {
                *b++ = 0;
                *b = 0;
            }
            else
                *a = 0;
        }
        else {
            int l = strlen(tstr);
            tstr[l+1] = 0;
        }

        double vv;
        if (ckt()->CKTvblk->query_var(tstr, 0, &vv)) {
            p->p_type = PT_PARAM;
            p->p_evfunc = &IFparseNode::p_vlparm;
            p->p_valname = tstr;
            return (p);
        }
        delete [] tstr;
    }

    // Deal with this later.
    p->p_type = PT_PLACEHOLDER;
    p->p_valname = lstring::copy(string);

    return (p);
}


IFparseNode *
IFparseTree::newFnode(const char *string, IFparseNode *arg)
{
    if (!string)
        return (0);

    IFparseNode *p = 0;
    if (lstring::cieq(string, "v")) {
        char *name = new char[128];
        if (arg->p_type == PT_PLACEHOLDER) {
            strcpy(name, arg->p_valname);
            delete [] arg->p_valname;
            arg->p_valname = 0;
        }
        else if (arg->p_type == PT_CONSTANT)
            snprintf(name, 128, "%d", (int)arg->v.constant);
        else if (arg->p_type != PT_COMMA) {
            Errs()->add_error("badly formed node voltage");
            delete [] name;
            return (0);
        }

        if (arg->p_type == PT_COMMA) {
            // Change v(a,b) into v(a) - v(b)
            delete [] name;
            IFparseNode *pl = newFnode(string, arg->p_left);
            IFparseNode *pr = newFnode(string, arg->p_right);
            p = newBnode(TT_MINUS, pl, pr);
        }
        else {
            int i = 0;
            if (ckt()) {
                IFdata temp;
                temp.type = IF_NODE;
                ckt()->termInsert(&name, &temp.v.nValue);
                int numvalues = num_vars();
                IFdata *values = vars();
                for (i = 0; i < numvalues; i++) {
                    if ((values[i].type & IF_VARTYPES) == IF_NODE &&
                            values[i].v.nValue == temp.v.nValue)
                        break;
                }
                if (i == numvalues) {
                    if (numvalues) {
                        IFdata *tmpv = new IFdata[numvalues + 1];
                        for (int j = 0; j < numvalues; j++)
                            tmpv[j] = values[j];
                        delete [] values;
                        values = tmpv;
                    }
                    else
                        values = new IFdata;

                    values[i] = temp;
                    numvalues++;
                }
                set_vars(values);
                set_num_vars(numvalues);
            }
            else
                delete [] name;

            p = newNode();
            p->p_valindx = i;
            p->p_type = PT_VAR;
            p->p_evfunc = &IFparseNode::p_var;
        }
        return (p);
    }

    if (lstring::cieq(string, "i")) {
        char *name = new char[128];
        if (arg->p_type == PT_PLACEHOLDER) {
            strcpy(name, arg->p_valname);
            delete [] arg->p_valname;
            arg->p_valname = 0;
        }
        else if (arg->p_type == PT_CONSTANT)
            snprintf(name, 128, "%d", (int)arg->v.constant);
        else {
            Errs()->add_error("badly formed branch current");
            delete [] name;
            return (0);
        }

        int i = 0;
        if (ckt()) {
            ckt()->insert(&name);
            int numvalues = num_vars();
            IFdata *values = vars();
            for (i = 0; i < numvalues; i++) {
                if ((values[i].type & IF_VARTYPES) == IF_INSTANCE &&
                        values[i].v.uValue == name)
                    break;
            }
            if (i == numvalues) {
                if (numvalues) {
                    IFdata *tmpv = new IFdata[numvalues + 1];
                    for (int j = 0; j < numvalues; j++)
                        tmpv[j] = values[j];
                    delete [] values;
                    values = tmpv;
                }
                else
                    values = new IFdata;

                values[i].v.uValue = (IFuid) name;
                values[i].type = IF_INSTANCE;
                numvalues++;
            }
            set_vars(values);
            set_num_vars(numvalues);
        }
        else
            delete [] name;

        p = newNode();
        p->p_valindx = i;
        p->p_type = PT_VAR;
        p->p_evfunc = &IFparseNode::p_var;
        return (p);
    }

    int i;
    for (i = 0; IFparseNode::PTfuncs[i].name; i++) {
        if (lstring::cieq(IFparseNode::PTfuncs[i].name, string))
            break;
    }
    if (IFparseNode::PTfuncs[i].name) {
        if (IFparseNode::PTfuncs[i].number == PTF_DERIV) {
            // derivative wrt "x"
            p = IFparseNode::copy(differentiate(arg, -1), true);
            return (p);
        }
        p = newNode();
        p->p_type = PT_FUNCTION;
        p->p_left = arg;
        p->p_valname = IFparseNode::PTfuncs[i].name;
        p->p_valindx = IFparseNode::PTfuncs[i].number;
        p->p_func = IFparseNode::PTfuncs[i].funcptr;
        p->p_evfunc = &IFparseNode::p_fcn;
        return (p);
    }

    if (ckt()) {
        // The function call name may refer to a table.
        sCKTtable *table;
        if (IP.tablFind(string, &table, ckt())) {

            p = newNode();
            p->p_type = PT_TFUNC;
            p->p_valindx = PTF_tTABLE;
            p->p_left = arg;
            p->p_valname = "table";
            p->v.table = table;
            p->p_evfunc = &IFparseNode::p_table;
            p->p_func = &IFparseNode::PTtTable;
            return (p);
        }

        // Give the user-defined functions a try.
        int arity = 0;
        if (arg) {
            p = arg;
            arity++;
            while (p && p->p_type == PT_COMMA) {
                p = p->p_right;
                arity++;
            }
        }

        IFmacro *macro = ckt()->find_macro(string, arity);
        if (!macro) {
            char *tf;
            const char *names;
            if (Sp.UserFunc(string, arity, &tf, &names)) {
                // Found a match, tf is function text (a copy), names is 0
                // separated argument list (don't free!).

                Errs()->init_error();
                macro = new IFmacro(string, arity, tf, names, ckt());
                ckt()->save_macro(macro);
            }
        }
        if (macro) {
            // Tran funcs can not be used in a macro.  A tran func node
            // must be unique for each argument set.  If the macro has
            // a tran func, we keep it in the table as a flag, but
            // otherwise don't use it.  Here, we link the macro text
            // into the existing tree.

            if (macro->has_tranfunc()) {
                const char *t = macro->text();
                p_push_args(arg, macro->argnames());
                p = treeParse(&t);
                p_pop_args();

                if (p) {
                    // success
                    return (p);
                }
                Errs()->add_error("error evaluating %s", string);
                return (0);
            }

            p = newNode();
            p->p_type = PT_MACRO;
            p->p_left = arg;
            p->p_valname = macro->name();
            p->p_valindx = arity;
            p->p_func = 0;
            p->p_evfunc = &IFparseNode::p_macro;
            p->v.macro = macro;
            return (p);
        }
    }

    Errs()->add_error("no such function, macro, or table '%s'", string);
    return (0);
}


// Push a transient argument map on a stack.  This is used when
// linking a macro into the calling tree.
//
void
IFparseTree::p_push_args(IFparseNode *arg, const char *names)
{
    pt_argmap = new sArgMap(arg, names, pt_argmap);
}


// Pop the last pushed argument map.
//
void
IFparseTree::p_pop_args()
{
    sArgMap *map = pt_argmap;
    if (pt_argmap)
        pt_argmap = pt_argmap->next;
    delete map;
}


// Make a constant node, used in differentiation only.
//
IFparseNode *
IFparseTree::p_mkcon(double value)
{
    IFparseNode *p = newNnode(value);
    p->p_newderiv = true;
    return (p);
}


// Make a binary operator node, used in differentiation only.
//
IFparseNode *
IFparseTree::p_mkb(int type, IFparseNode *left, IFparseNode *right)
{
    if (!left || !right)
        return (0);
    if ((right->p_type == PT_CONSTANT) && (left->p_type == PT_CONSTANT)) {
        switch (type) {
        case PT_TIMES:
            return (p_mkcon(left->v.constant * right->v.constant));
        case PT_DIVIDE:
            return (p_mkcon(left->v.constant / right->v.constant));
        case PT_PLUS:
            return (p_mkcon(left->v.constant + right->v.constant));
        case PT_MINUS:
            return (p_mkcon(left->v.constant - right->v.constant));
        case PT_POWER:
            return (p_mkcon(pow(left->v.constant, right->v.constant)));
        }
    }

    switch (type) {
    case PT_TIMES:
        if ((left->p_type == PT_CONSTANT) && (left->v.constant == 0))
            return (left);
        else if ((right->p_type == PT_CONSTANT) && (right->v.constant == 0))
            return (right);
        else if ((left->p_type == PT_CONSTANT) && (left->v.constant == 1))
            return (right);
        else if ((right->p_type == PT_CONSTANT) && (right->v.constant == 1))
            return (left);
        break;

    case PT_DIVIDE:
        if ((left->p_type == PT_CONSTANT) && (left->v.constant == 0))
            return (left);
        else if ((right->p_type == PT_CONSTANT) && (right->v.constant == 1))
            return (left);
        break;

    case PT_PLUS:
        if ((left->p_type == PT_CONSTANT) && (left->v.constant == 0))
            return (right);
        else if ((right->p_type == PT_CONSTANT) && (right->v.constant == 0))
            return (left);
        break;

    case PT_MINUS:
        if ((right->p_type == PT_CONSTANT) && (right->v.constant == 0))
            return (left);
        else if ((left->p_type == PT_CONSTANT) && (left->v.constant == 0))
            return (p_mkf(PTF_UMINUS, right));
        break;

    case PT_POWER:
        if (right->p_type == PT_CONSTANT) {
            if (right->v.constant == 0)
                return (p_mkcon(1.0));
            else if (right->v.constant == 1)
                return (left);
        }
        break;
    }

    int i;
    for (i = 0; IFparseNode::PTops[i].name; i++)
        if (IFparseNode::PTops[i].number == type)
            break;
    if (!IFparseNode::PTops[i].name) {
        Errs()->add_error("bad operator index %d", type);
        return (0);
    }
    IFparseNode *p = newNode();
    p->p_newderiv = true;
    p->p_type = (PTtype)type;
    p->p_left = left;
    p->p_right = right;
    p->p_func = IFparseNode::PTops[i].funcptr;
    p->p_valname = IFparseNode::PTops[i].name;
    p->p_evfunc = &IFparseNode::p_op;
    return (p);
}


// Make a function node, used in differentiation only.
//
IFparseNode *
IFparseTree::p_mkf(int type, IFparseNode *arg)
{
    if (!arg)
        return (0);
    int i;
    for (i = 0; IFparseNode::PTfuncs[i].name; i++)
        if (IFparseNode::PTfuncs[i].number == type)
            break;
    if (!IFparseNode::PTfuncs[i].name) {
        Errs()->add_error("bad function index %d", type);
        return (0);
    }

    if (arg->p_type == PT_CONSTANT) {
        double constval =
            (arg->*IFparseNode::PTfuncs[i].funcptr)(&arg->v.constant);
        return (p_mkcon(constval));
    }
    IFparseNode *p = newNode();
    p->p_newderiv = true;
    p->p_type = PT_FUNCTION;
    p->p_left = arg;
    p->p_valindx = IFparseNode::PTfuncs[i].number;;
    p->p_func = IFparseNode::PTfuncs[i].funcptr;
    p->p_valname = IFparseNode::PTfuncs[i].name;
    p->p_evfunc = &IFparseNode::p_fcn;
    return (p);
}


// Differentiate the "tran" function node.
//
IFparseNode *
IFparseTree::p_mktrand(IFparseNode *p, int varnum)
{
    if (p->p_valindx == PTF_tTABLE) {
        IFparseNode *arg = differentiate(p->p_left, varnum);
        if (arg) {
            IFparseNode *np = IFparseNode::copy(p);
            np->p_newderiv = true;
            np->p_func = &IFparseNode::PTtTableD;
            return (p_mkb(PT_TIMES, np, arg));
        }
        return (0);
    }
    if (varnum < 0 && !pt_num_xvars) {
        // time derivative (only)
        IFparseNode *np = IFparseNode::copy(p);
        np->p_newderiv = true;
        np->p_func = &IFparseNode::tran_deriv;
        return (np);
    }
    if (p->p_valindx == PTF_tPWL && pt_num_xvars) {
        if (varnum == 0) {
            IFparseNode *np = IFparseNode::copy(p);
            np->p_newderiv = true;
            np->p_func = &IFparseNode::tran_deriv;
            return (np);
        }
        if (varnum == 1 && pt_num_xvars == 2) {
            IFparseNode *np = IFparseNode::copy(p);
            np->p_newderiv = true;
            np->p_func = &IFparseNode::tran_deriv;
            np = p_mkb(PT_TIMES, p_mkcon(-1.0), np);
            return (np);
        }
    }
    return (0);
}
// End of IFparseTree functions.


// Given a pointer to an element, make a pnode out of it (if it already
// is one, return a pointer to it). If it isn't of type VALUE, then return
// 0.
//
IFparseNode *
PTelement::makeNode(void *uarg)
{
    if (token != TT_VALUE)
        return (0);
    if (type == DT_STRING || type == DT_USTRING) {
        // If the string indicates a branch current, parse as i(dev),
        // to get the correct behavior in this case.

        char *b = strchr(vu.string, '#');
        if (b && lstring::cieq(b+1, "branch")) {
            *b = 0;
            IFparseNode *a = makeSnode(uarg);
            vu.string = lstring::copy("i");
            return (makeFnode(a, uarg));
        }
        return (makeSnode(uarg));
    }
    if (type == DT_NUM)
        return (makeNnode(uarg));
    if (type == DT_PNODE)
        return (vu.node);
    Errs()->add_error("bad token type %d", type);
    return (0);
}


// Number node.
//
IFparseNode *
PTelement::makeNnode(void *uarg)
{
    IFparseTree *tree = (IFparseTree*)uarg;
    if (!tree)
        return (0);
    return (tree->newNnode(vu.real));
}


IFparseNode *
PTelement::makeUnode(IFparseNode *arg, void *uarg)
{
    IFparseTree *tree = (IFparseTree*)uarg;
    if (!tree)
        return (0);
    return (tree->newUnode(token, arg));
}


// Binop node.
//
IFparseNode *
PTelement::makeBnode(IFparseNode *arg1, IFparseNode *arg2, void *uarg)
{
    IFparseTree *tree = (IFparseTree*)uarg;
    if (!tree)
        return (0);
    return (tree->newBnode(token, arg1, arg2));
}


// String node, string is freed (or used)
//
IFparseNode *
PTelement::makeSnode(void *uarg)
{
    IFparseTree *tree = (IFparseTree*)uarg;
    if (!tree)
        return (0);
    IFparseNode *p = tree->newSnode(vu.string);
    delete [] vu.string;
    vu.string = 0;
    return (p);
}


IFparseNode *
PTelement::makeFnode(IFparseNode *arg, void *uarg)
{
    IFparseTree *tree = (IFparseTree*)uarg;
    if (!tree)
        return (0);
    IFparseNode *p = tree->newFnode(vu.string, arg);
    delete [] vu.string;
    vu.string = 0;
    return (p);
}


// Here we recognize the 'tran' functions only.
//
char *
PTelement::userString(const char **s, bool in_source)
{
    return (IP.getTranFunc(s, in_source));
}
// End of PTelement functions.


IFparseNode::~IFparseNode()
{
    if (p_type == PT_TFUNC)
        delete v.td;
    else if (p_type == PT_PARAM) {
        delete [] p_valname;
        if (p_valindx < 0 && p_evfunc == &IFparseNode::p_parm)
            delete v.sp;
    }
    else if (p_type == PT_PLACEHOLDER || p_type == PT_MACROARG)
        delete [] p_valname;
    else if (p_type == PT_MACRO_DERIV)
        delete v.macro_deriv;
}


// Static function.
// Check for remaining PT_PLACEHOLDERs in the parse tree.
// Returns true if ok.
//
bool
IFparseNode::check(const IFparseNode *pn)
{
    if (!pn) {
        Errs()->add_error("parse error, null parse node");
        return (false);
    }
    switch (pn->p_type) {
    case PT_PLACEHOLDER:
        return (false);

    case PT_CONSTANT:
    case PT_VAR:
    case PT_PARAM:
    case PT_TFUNC:
        return (true);

    case PT_FUNCTION:
    case PT_MACRO:
        return (check(pn->p_left));

    case PT_PLUS:
    case PT_MINUS:
    case PT_TIMES:
    case PT_DIVIDE:
    case PT_POWER:
    case PT_COMMA:
#ifdef NEWOPS
    case PT_EQ:
    case PT_GT:
    case PT_LT:
    case PT_GE:
    case PT_LE:
    case PT_NE:
    case PT_AND:
    case PT_OR:
    case PT_NOT:
#endif
        return (check(pn->p_left) && check(pn->p_right));

    case PT_MACRO_DERIV:
        return (true);

    default:
        break;
    }

    Errs()->add_error("bad node type %d", pn->p_type);
    return (false);
}


// Static function.
// Variation for macro checking.  Fails if null or bad node, if
// PT_PLACEHOLER is found, or PT_VAR.  The latter can't be resolved at
// run time (presently).
//
bool
IFparseNode::check_macro(const IFparseNode *pn)
{
    if (!pn)
        return (false);

    switch (pn->p_type) {
    case PT_PLACEHOLDER:
    case PT_VAR:
        return (false);

    case PT_CONSTANT:
    case PT_PARAM:
    case PT_TFUNC:
    case PT_MACROARG:
        return (true);

    case PT_FUNCTION:
    case PT_MACRO:
        return (check_macro(pn->p_left));

    case PT_PLUS:
    case PT_MINUS:
    case PT_TIMES:
    case PT_DIVIDE:
    case PT_POWER:
    case PT_COMMA:
        return (check_macro(pn->p_left) && check_macro(pn->p_right));

    default:
        break;
    }
    return (false);
}


// Static function.
// Return true if the tree is a constant expression.
//
bool
IFparseNode::is_const(const IFparseNode *pn)
{
    if (pn) {
        if (pn->p_type == PT_PLACEHOLDER || pn->p_type == PT_VAR ||
                pn->p_type == PT_PARAM || pn->p_type == PT_TFUNC)
            return (false);
        if (!is_const(pn->p_left))
            return (false);
        if (!is_const(pn->p_right))
            return (false);
    }
    return (true);
}


// Evaluate and trim out constant parts of the tree.  The argument is the
// address of this.
//
void
IFparseNode::collapse(IFparseNode **pp)
{
    if (p_type == PT_TFUNC) {
        // The arguments are all constants, and there can be a lot of
        // them (e.g., from a long PWL).  Recursion can cause a fault
        // due to stack overflow!  Nothing to do here anyway.
        return;
    }
    if (p_left)
        p_left->collapse(&p_left);
    if (p_right)
        p_right->collapse(&p_right);

    if (p_type == PT_PLUS) {
        if (!p_left || !p_right)
            return;
        if (p_left->p_type == PT_CONSTANT && p_right->p_type == PT_CONSTANT) {
            p_type = PT_CONSTANT;
            v.constant = p_left->v.constant + p_right->v.constant;
            p_evfunc = &IFparseNode::p_const;
            p_valname = 0;
            p_valindx = 0;
            p_left = 0;
            p_right = 0;
        }
        else if (p_left->p_type == PT_CONSTANT && p_left->v.constant == 0.0) {
            *pp = p_right;
            p_right = 0;
        }
        else if (p_right->p_type == PT_CONSTANT && p_right->v.constant == 0.0) {
            *pp = p_left;
            p_left = 0;
        }
    }
    else if (p_type == PT_MINUS) {
        if (!p_left || !p_right)
            return;
        if (p_left->p_type == PT_CONSTANT && p_right->p_type == PT_CONSTANT) {
            p_type = PT_CONSTANT;
            v.constant = p_left->v.constant - p_right->v.constant;
            p_evfunc = &IFparseNode::p_const;
            p_valname = 0;
            p_valindx = 0;
            p_left = 0;
            p_right = 0;
        }
        else if (p_left->p_type == PT_CONSTANT && p_left->v.constant == 0.0) {
            // Convert to unary minus (a function call).
            p_type = PT_FUNCTION;
            p_left = p_right;
            p_right = 0;
            p_valname = "-";
            p_valindx = PTF_UMINUS;
            p_func = &IFparseNode::PTuminus;
            p_evfunc = &IFparseNode::p_fcn;
        }
        else if (p_right->p_type == PT_CONSTANT && p_right->v.constant == 0.0) {
            *pp = p_left;
            p_left = 0;
        }
    }
    else if (p_type == PT_TIMES) {
        if (!p_left || !p_right)
            return;
        if (p_left->p_type == PT_CONSTANT && p_right->p_type == PT_CONSTANT) {
            p_type = PT_CONSTANT;
            v.constant = p_left->v.constant * p_right->v.constant;
            p_evfunc = &IFparseNode::p_const;
            p_valname = 0;
            p_valindx = 0;
            p_left = 0;
            p_right = 0;
        }
        else if (p_left->p_type == PT_CONSTANT) {
            if (p_left->v.constant == 1.0) {
                *pp = p_right;
                p_right = 0;
            }
            else if (p_left->v.constant == 0.0) {
                *pp = p_left;
                p_left = 0;
            }
        }
        else if (p_right->p_type == PT_CONSTANT) {
            if (p_right->v.constant == 1.0) {
                *pp = p_left;
                p_left = 0;
            }
            else if (p_right->v.constant == 0.0) {
                *pp = p_right;
                p_right = 0;
            }
        }
    }
    else if (p_type == PT_DIVIDE) {
        if (!p_left || !p_right)
            return;
        if (p_left->p_type == PT_CONSTANT && p_right->p_type == PT_CONSTANT) {
            p_type = PT_CONSTANT;
            v.constant = p_left->v.constant / p_right->v.constant;
            p_evfunc = &IFparseNode::p_const;
            p_valname = 0;
            p_valindx = 0;
            p_left = 0;
            p_right = 0;
        }
        else if (p_right->p_type == PT_CONSTANT && p_right->v.constant == 1.0) {
            *pp = p_left;
            p_left = 0;
        }
        else if (p_left->p_type == PT_CONSTANT && p_left->v.constant == 0.0) {
            *pp = p_left;
            p_left = 0;
        }
    }
    else if (p_type == PT_POWER) {
        if (!p_left || !p_right)
            return;
        if (p_left->p_type == PT_CONSTANT && p_right->p_type == PT_CONSTANT) {
            p_type = PT_CONSTANT;
            v.constant = pow(p_left->v.constant, p_right->v.constant);
            p_evfunc = &IFparseNode::p_const;
            p_valname = 0;
            p_valindx = 0;
            p_left = 0;
            p_right = 0;
        }
    }
    else if (p_type == PT_FUNCTION) {
        if (!p_left)
            return;
        if (p_left->p_type == PT_CONSTANT) {
            p_type = PT_CONSTANT;
            v.constant = (this->*p_func)(&p_left->v.constant);
            p_evfunc = &IFparseNode::p_const;
            p_valname = 0;
            p_valindx = 0;
            p_func = 0;
            p_left = 0;
            p_right = 0;
        }
        else if (p_left->p_type == PT_COMMA) {
            if (!p_left->p_left || !p_left->p_right)
                return;
            if (p_left->p_left->p_type == PT_CONSTANT &&
                    p_left->p_right->p_type == PT_CONSTANT) {
                v.constant = p_left->p_left->v.constant;
                v.constant = (this->*p_func)(&p_left->p_right->v.constant);
                p_type = PT_CONSTANT;
                p_evfunc = &IFparseNode::p_const;
                p_valname = 0;
                p_valindx = 0;
                p_func = 0;
                p_left = 0;
                p_right = 0;
            }
        }
    }
    else if (p_type == PT_MACRO) {
        if (!p_left)
            return;
        if (v.macro->has_tranfunc())
            return;
        bool doit = (p_left->p_type == PT_CONSTANT);
        if (!doit && p_left->p_type == PT_COMMA) {
            IFparseNode *px = p_left;
            doit = true;
            while (px) {
                if (px->p_left->p_type != PT_CONSTANT) {
                    doit = false;
                    break;
                }
                if (px->p_right->p_type == PT_CONSTANT)
                   break;
                if (px->p_right->p_type == PT_COMMA) {
                    px = px->p_right;
                    continue;
                }
                // bad argument list
                doit = false;
                break;
            }
        }
        if (doit) {
            (this->*IFparseNode::p_evfunc)(&v.constant, 0, 0);
            p_type = PT_CONSTANT;
            p_evfunc = &IFparseNode::p_const;
            p_valname = 0;
            p_valindx = 0;
            p_left = 0;
            p_right = 0;
        }
    }
}


// Search the tree, for each PT_PLACEHOLDER that matches a name in the
// null-separated names list, convert the node into a PT_MACROARG with
// an offset.
//
void
IFparseNode::set_args(const char *names)
{
    if (p_type == PT_TFUNC)
        return;
    if (p_left)
        p_left->set_args(names);
    if (p_right)
        p_right->set_args(names);

    if (p_type == PT_PLACEHOLDER) {
        const char *t = names;
        int i = 0;
        while (*t) {
            if (lstring::cieq(t, p_valname)) {
                p_type = PT_MACROARG;
                p_valindx = i;
                p_evfunc = &IFparseNode::p_macarg;
                return;
            }
            while (*t)
                t++;
            t++;
            i++;
        }
    }
}


// Return a limit for the transient analysis time step.  Limiting is
// needed for, e.g., sin(), which sets no breakpoints.
//
double
IFparseNode::time_limit(sCKT *ckt, double lim) 
{
    if (p_type == PT_TFUNC) {
        if (v.td)
            v.td->time_limit(ckt, &lim);

        // No need to check args, all are constants.  In fact,
        // recursing through the args may cause a seg fault due to
        // stack overrun for (e.g.) very long pwl argument lists.
    }
    else {
        if (p_left) { 
            double lx = p_left->time_limit(ckt, lim); 
            if (lx < lim)
                lim = lx;
        }
        if (p_right) {
            double lx = p_right->time_limit(ckt, lim);
            if (lx < lim)
                lim = lx;
        }
    }
    return (lim);
}


// Print the tree expression in lstr.
//
void
IFparseNode::print_string(sLstr &lstr, TokenType parent_type, bool rhs)
{
    if (p_type == PT_CONSTANT)
        lstr.add_g(v.constant);
    else if (p_type == PT_VAR)
        p_tree->varName(p_valindx, lstr);
    else if (p_type == PT_PARAM)
        lstr.add(specSigs[p_valindx]);
    else if (p_type == PT_PLUS) {
        bool prn = Parser::parenTable(TT_PLUS, parent_type, rhs);
        if (prn)
            lstr.add_c('(');
        p_left->print_string(lstr, TT_PLUS, false);
        lstr.add(" + ");
        p_right->print_string(lstr, TT_PLUS, true);
        if (prn)
            lstr.add_c(')');
    }
    else if (p_type == PT_MINUS) {
        bool prn = Parser::parenTable(TT_MINUS, parent_type, rhs);
        if (prn)
            lstr.add_c('(');
        p_left->print_string(lstr, TT_MINUS, false);
        lstr.add(" - ");
        p_right->print_string(lstr, TT_MINUS, true);
        if (prn)
            lstr.add_c(')');
    }
    else if (p_type == PT_TIMES) {
        bool prn = Parser::parenTable(TT_TIMES, parent_type, rhs);
        if (prn)
            lstr.add_c('(');
        p_left->print_string(lstr, TT_TIMES, false);
        lstr.add("*");
        p_right->print_string(lstr, TT_TIMES, true);
        if (prn)
            lstr.add_c(')');
    }
    else if (p_type == PT_DIVIDE) {
        bool prn = Parser::parenTable(TT_DIVIDE, parent_type, rhs);
        if (prn)
            lstr.add_c('(');
        p_left->print_string(lstr, TT_DIVIDE, false);
        lstr.add("/");
        p_right->print_string(lstr, TT_DIVIDE, true);
        if (prn)
            lstr.add_c(')');
    }
    else if (p_type == PT_COMMA) {
        p_left->print_string(lstr);
        lstr.add(", ");
        p_right->print_string(lstr);
    }
    else if (p_type == PT_POWER) {
        bool prn = Parser::parenTable(TT_POWER, parent_type, rhs);
        if (prn)
            lstr.add_c('(');
        p_left->print_string(lstr, TT_POWER, false);
        lstr.add("^");
        p_right->print_string(lstr, TT_POWER, true);
        if (prn)
            lstr.add_c(')');
    }
    else if (p_type == PT_FUNCTION || p_type == PT_MACRO) {
        if (*p_valname == '-' && !*(p_valname+1)) {
            // Unary minus.
            lstr.add(p_valname);
            p_left->print_string(lstr, TT_UMINUS);
        }
        else {
            lstr.add(p_valname);
            lstr.add_c('(');
            p_left->print_string(lstr);
            lstr.add_c(')');
        }
    }
    else if (p_type == PT_MACRO_DERIV) {
        lstr.add("D[");
        lstr.add(v.macro_deriv->md_macro->name());
        lstr.add("](");
        p_left->print_string(lstr);
        lstr.add_c(')');
    }
    else if (p_type == PT_TFUNC) {
        if (v.td)
            v.td->print(p_valname, lstr);
    }
    else if (p_type == PT_PLACEHOLDER || p_type == PT_MACROARG)
        lstr.add(p_valname);
    else
        lstr.add("(bad node)");
}


// Return a string representation of the node tree.  If len is
// positive, the string is truncated at len characters and "..." is
// added.
//
char *
IFparseNode::get_string(int len)
{
    sLstr lstr;
    print_string(lstr);
    if (len > 0)
        lstr.truncate(len, "..."); 
    return (lstr.string_trim());
}


//
// Evaluation functions.
//

int
IFparseNode::p_const(double *res, const double*, const double*)
{
    *res = v.constant;
    return (OK);
}


int
IFparseNode::p_var(double *res, const double *vals, const double*)
{
    *res = vals[p_valindx];
    return (OK);
}


int
IFparseNode::p_parm(double *res, const double *vals, const double *macargs)
{
    switch (p_valindx) {
    case SPEC_TIME:
        *res = p_tree->ckt()->CKTtime;
        return (OK);
    case SPEC_FREQ:
        *res = p_tree->ckt()->CKTomega/(2*M_PI);
        return (OK);
    case SPEC_OMEG:
        *res = p_tree->ckt()->CKTomega;
        return (OK);
    }

    // Must be a device/model/etc. parameter.
    if (!v.sp)
        v.sp = new IFspecial;
    if (v.sp->sp_error == OK) {
        int i = 0;
        if (p_left) {
            // index for list
            double r;
            int err = (p_left->*p_left->p_evfunc)(&r, vals, macargs);
            if (err) {
                char *nstr = get_string(64);
                Errs()->add_error(
                    "parameter index evaluation failed\n  node: %s", nstr);
                delete [] nstr;
                v.sp->sp_error = err;
            }
            else
                i = (int)r;
        }
        IFdata data;
        v.sp->evaluate(p_valname, p_tree->ckt(), &data, i);
        if (v.sp->sp_error == OK) {
            if ((data.type & IF_VARTYPES) == IF_REAL)
                *res = data.v.rValue;
            else if ((data.type & IF_VARTYPES) == IF_INTEGER)
                *res = (double)data.v.iValue;
            else if ((data.type & IF_VARTYPES) == IF_COMPLEX)
                *res = data.v.cValue.real;
            else
                *res = 0.0;
        }
        else
            *res = 0.0;
    }
    return (OK);
}


int
IFparseNode::p_vlparm(double *res, const double*, const double*)
{
    double vv;
    // range starts after null, double null if no range
    const char *rng = p_valname + strlen(p_valname) + 1;
    if (p_tree && p_tree->ckt() && p_tree->ckt()->CKTvblk &&
            p_tree->ckt()->CKTvblk->query_var(p_valname, rng, &vv)) {
        if (vv != p_auxval) {
            p_auxtime = p_tree->ckt()->CKTtime;
            if (p_auxtime == 0.0)
                p_auxval = vv;
            v.constant = p_auxval;
            p_auxval = vv;
        }
        if (p_tree->ckt()->CKTtime - p_auxtime < p_tree->ckt()->CKTstep)
            *res = v.constant + (vv - v.constant)*
                (p_tree->ckt()->CKTtime - p_auxtime)/p_tree->ckt()->CKTstep;
        else
            *res = vv;
    }
    else
        *res = 0.0;
    return (OK);
}


int
IFparseNode::p_op(double *res, const double *vals, const double *macargs)
{
    double r[2];
    int err = (p_left->*p_left->p_evfunc)(&r[0], vals, macargs);
    if (err != OK)
        return (err);
    err = (p_right->*p_right->p_evfunc)(&r[1], vals, macargs);
    if (err != OK)
        return (err);
    if (p_type == PT_COMMA) {
        char *nstr = get_string(64);
        Errs()->add_error("syntax error, unexpected comma\n  node: %s", nstr);
        delete [] nstr;
        return (E_SYNTAX);
    }
    *res = (this->*p_func)(r);
    return (OK);
}


int
IFparseNode::p_fcn(double *res, const double *vals, const double *macargs)
{
    if (p_left->p_type == PT_COMMA) {
        double r[2];
        if (!p_two_args(p_valindx)) {
            char *nstr = get_string(64);
            Errs()->add_error(
                "incorrect argument count to function\n  node: %s", nstr);
            delete [] nstr;
            return (E_BADPARM);
        }
        IFparseNode *p = p_left->p_left;
        int err = (p->*p->p_evfunc)(&r[0], vals, macargs);
        if (err != OK)
            return (err);
        p = p_left->p_right;
        if (p->p_type == PT_COMMA) {
            char *nstr = get_string(64);
            Errs()->add_error(
                "incorrect argument count to function\n  node: %s", nstr);
            delete [] nstr;
            return (E_BADPARM);
        }
        err = (p->*p->p_evfunc)(&r[1], vals, macargs);
        if (err != OK) {
            char *nstr = get_string(64);
            Errs()->add_error("function evaluation failed\n  node: %s", nstr);
            delete [] nstr;
            return (err);
        }
        *res = (this->*p_func)(r);
    }
    else {
        double r1;
        int err = (p_left->*p_left->p_evfunc)(&r1, vals, macargs);
        if (err != OK) {
            char *nstr = get_string(64);
            Errs()->add_error("function evaluation failed\n  node: %s", nstr);
            delete [] nstr;
            return (err);
        }
        *res = (this->*p_func)(&r1);
    }
    return (OK);
}


int
IFparseNode::p_tran(double *res, const double *vals, const double *macargs)
{
    double x;
    // If the "xalias" is defined, this will be used in PWL, otherwise,
    // time is used
    if (p_type == PT_TFUNC && p_valindx == PTF_tPWL &&
            p_tree && p_tree->xtree()) {
        int err = (p_tree->xtree()->*p_tree->xtree()->p_evfunc)(&x, vals,
            macargs);
        if (err != OK) {
            char *nstr = get_string(64);
            Errs()->add_error("PWL evaluation failed\n  node: %s", nstr);
            delete [] nstr;
            return (err);
        }
    }
    else
        x = p_tree->ckt()->CKTtime;
    *res = (this->*p_func)(&x);
    return (OK);
}


int
IFparseNode::p_table(double *res, const double *vals, const double *macargs)
{
    double r1;
    int err = (p_left->*p_left->p_evfunc)(&r1, vals, macargs);
    if (err != OK) {
        char *nstr = get_string(64);
        Errs()->add_error("table evaluation failed\n  node: %s", nstr);
        delete [] nstr;
        return (err);
    }
    *res = (this->*p_func)(&r1);
    return (OK);
}


int
IFparseNode::p_macarg(double *res, const double*, const double *macargs)
{
    *res = (macargs ? macargs[p_valindx] : 0.0);
    return (OK);
}


// Evaluation function for a macro.  Macros are "user defined functions"
// obtained from the front-end.  Macros are kept as separate trees to
// minimize memory consumption, as parse trees can grow huge otherwise,
// i.e., if macros are expanded and linked into the calling tree.
//
int
IFparseNode::p_macro(double *res, const double *vals, const double *macargs)
{
    IFmacro *macro = v.macro;
    if (macro) {
        double *args = (double*)alloca((macro->numargs() + 1)*sizeof(double));
        int error = macro->set_args(p_left, vals, macargs, args);
        if (error) {
            char *nstr = get_string(64);
            Errs()->add_error("macro argument setup failed\n  node: %s",
                nstr);
            delete [] nstr;
            return (error);
        }
        error = macro->evaluate(res, vals, args);
        if (error) {
            char *nstr = get_string(64);
            Errs()->add_error("macro evaluation failed\n  node: %s", nstr);
            delete [] nstr;
            return (error);
        }
        return (OK);
    }
    char *nstr = get_string(64);
    Errs()->add_error("macro not found\n  node: %s", nstr);
    delete [] nstr;
    return (E_NOTFOUND);
}


// Evaluation function for the derivative of a macro.  We need to return
// sum_over_args { d_macro/d_arg * d_arg/d_val }
// where val is the circuit variable (e.g., node voltage).
//
int
IFparseNode::p_macro_deriv(double *res, const double *vals,
    const double *macargs)
{
    IFmacroDeriv *md = v.macro_deriv;
    if (!md) {
        char *nstr = get_string(64);
        Errs()->add_error("macro derivative not found\n  node: %s", nstr);
        delete [] nstr;
        return (E_NOTFOUND);
    }
    IFmacro *macro = md->md_macro;
    if (!macro) {
        char *nstr = get_string(64);
        Errs()->add_error("macro not found\n  node: %s", nstr);
        delete [] nstr;
        return (E_NOTFOUND);
    }

    double *args = (double*)alloca((macro->numargs() + 1)*sizeof(double));
    int error = macro->set_args(p_left, vals, macargs, args);
    if (error) {
        char *nstr = get_string(64);
        Errs()->add_error("macro argument setup failed\n  node: %s",
            nstr);
        delete [] nstr;
        return (error);
    }
    double r = 0.0;
    for (int i = 0; i < macro->numargs(); i++) {

        double darg;
        IFparseNode *p = md->md_argdrvs[i];
        error = (p->*p->p_evfunc)(&darg, vals, args);
        if (error) {
            char *nstr = get_string(64);
            Errs()->add_error(
                "macro argument derivative evaluation failed\n  node: %s",
                nstr);
            delete [] nstr;
            return (error);
        }
        if (darg == 0.0)
            continue;

        double dmac;
        p = macro->tree()->derivs()[i];
        error = (p->*p->p_evfunc)(&dmac, vals, args);
        if (error) {
            char *nstr = get_string(64);
            Errs()->add_error(
                "macro derivative evaluation failed\n  node: %s", nstr);
            delete [] nstr;
            return (error);
        }

        r += darg*dmac;
    }
    *res = r;
    return (OK);
}


// Private copy function, accessed publicly through a static wrapper.
//
IFparseNode *
IFparseNode::copy_prv(bool skip_nd)
{
    // The p_newderiv flag is set if the node was created in differentiation,
    // so we don't have to copy it, just the args, during the final copy
    // of the differentiated tree.

    if (skip_nd && p_newderiv) {
        p_newderiv = false;
        p_left = copy(p_left);
        p_right = copy(p_right);
        return (this);
    }

    IFparseNode *newp = p_tree->newNode();
    *newp = *this;
    newp->p_newderiv = false;
    if (newp->p_type == PT_TFUNC) {
        if (p_valindx != PTF_tTABLE)
            newp->v.td = v.td ? v.td->dup() : 0;
    }
    else if (p_type == PT_PARAM || p_type == PT_PLACEHOLDER ||
            p_type == PT_MACROARG) {
        newp->p_valname = lstring::copy(p_valname);
        if (p_type == PT_PARAM && p_valindx < 0 &&
                p_evfunc == &IFparseNode::p_parm) {
            newp->v.sp = new IFspecial;
            *newp->v.sp = *v.sp;
        }
    }
    else if (p_type == PT_MACRO_DERIV) {
        IFmacroDeriv *mdold = v.macro_deriv;
        IFmacroDeriv *md = new IFmacroDeriv(*mdold);
        newp->v.macro_deriv = md;
        for (int i = 0; i < md->md_macro->numargs(); i++)
            md->md_argdrvs[i] = copy(mdold->md_argdrvs[i], skip_nd);
    }

    newp->p_left = copy(p_left);
    newp->p_right = copy(p_right);
    return (newp);
}


void
IFparseNode::p_init_node(double step, double finaltime)
{
    // Don't recurse into tranfunc args, unnecessary and can cause
    // stack overflow for long lists.
    if (p_type != PT_TFUNC) {
        if (p_left)
            p_left->p_init_node(step, finaltime);
    }
    if (p_right)
        p_right->p_init_node(step, finaltime);
    p_init_func(step, finaltime);
}


void
IFparseNode::p_init_func(double step, double finaltime, bool skipbr)
{
    // Only needed for "tran" funcs.
    if (p_type == PT_TFUNC && v.td) {
        // Skip breakpoint setting if time is not independent variable.
        if (p_valindx == PTF_tPWL && p_tree->num_xvars())
            skipbr = true;

        v.td->setup(p_tree->ckt(), step, finaltime, skipbr);
    }
}
// End of IFparseNode functions.


namespace {
    bool check_tranfunc(IFparseNode *p)
    {
        if (p->left()) {
            if (check_tranfunc(p->left()))
                return (true);
        }
        if (p->right()) {
            if (check_tranfunc(p->right()))
                return (true);
        }
        return (p->type() == PT_TFUNC);
    }
}


IFmacro::IFmacro(const char *mname, int arity, const char *mtext,
    const char *names, sCKT *ckt)
{
    m_name = lstring::copy(mname);
    m_numargs = arity;
    m_text = 0;
    m_argnames = 0;
    m_has_tranfunc = false;

    const char *t = mtext;
    m_tree = IFparseTree::getTree(&t, ckt, 0, true);
    if (!m_tree) {
        Errs()->add_error("parse failed for macro %s.", mname);
        return;
    }
    m_text = new char[t - mtext + 1];
    strncpy(m_text, mtext, t - mtext);
    m_text[t - mtext] = 0;

    m_has_tranfunc = check_tranfunc(m_tree->tree());
    if (m_has_tranfunc) {
        // We can't use this one normally, but will use it to keep
        // some information.  Copy the name list.
        t = names;
        while (*t) {
            while (*t)
                t++;
            t++;
        }
        m_argnames = new char[t - names + 1];
        memcpy(m_argnames, names, t - names + 1);
        return;
    }

    m_tree->tree()->set_args(names);
    if (!IFparseNode::check_macro(m_tree->tree())) {
        // If v(xxx) or similar appears explicitly in a macro, it will
        // not be resolved at present.  So, this is currently not
        // allowed.

        Errs()->add_error("macro %s contains unresolved references.", mname);
        delete m_tree;
        m_tree = 0;
        return;
    }

    if (m_numargs > 0) {
        // Compute DF/Darg for each arg, and save these in the tree.

        IFparseNode **derivs = new IFparseNode*[m_numargs];
        for (int i = 0; i < m_numargs; i++) {
            IFparseNode *p = m_tree->differentiate(m_tree->tree(), i);
            if (p) {
                p->collapse(&p);
                derivs[i] = IFparseNode::copy(p, true);
            }
            else {
                derivs[i] = m_tree->newNnode(0.0);
                Errs()->add_error(
                    "differentiation of macro %s for variable %d failed,\n"
                    "setting derivative to 0.", m_name, i + 1);
            }
        }
        tree()->set_derivs(m_numargs, derivs);
    }
}


IFmacro::~IFmacro()
{
    delete m_tree;
    delete [] m_name;
    delete [] m_text;
    delete [] m_argnames;
}


// Fill in the arguments array for the macro by evaluating the
// arguments.  The p is the argument list (p_left of the calling
// node).  The args points to a buffer of m_numargs doubles.
//
int
IFmacro::set_args(IFparseNode *p, const double *vals, const double *macargs,
    double *args)
{
    int i = 0;
    if (m_has_tranfunc)
        return (OK);
    while (p) {
        if (i == m_numargs) {
            // Too many args.
            return (E_SYNTAX);
        }
        double r;
        if (p->p_type == PT_COMMA) {
            int error = (p->p_left->*p->p_left->p_evfunc)(&r, vals, macargs);
            if (error)
                return (error);
            args[i++] = r;
            p = p->p_right;
            continue;
        }
        int error = (p->*p->p_evfunc)(&r, vals, macargs);
        if (error)
            return (error);
        args[i++] = r;
        if (i < m_numargs) {
            // Too few args;
            return (E_SYNTAX);
        }
        break;
    }
    return (OK);
}


int
IFmacro::evaluate(double *res, const double *vals, const double *macargs)
{
    return ((m_tree->tree()->*m_tree->tree()->p_evfunc)(res, vals, macargs));
}
// End of IFmacro functions.

