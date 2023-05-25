
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "simulator.h"
#include "parser.h"
#include "output.h"
#include "input.h"
#include "ttyio.h"
#include "errors.h"
#include "wlist.h"
#include "spnumber/paramsub.h"
#include "ginterf/graphics.h"
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef INT_MAX
#define INT_MAX     2147483646
#endif


//
// A simple operator-precedence parser for algebraic expressions.
// This also handles relational and logical expressions.
//

// the elements for the parser
struct spElement : public Element
{
    pnode *makeNode(void*);
    pnode *makeBnode(pnode*, pnode*, void*);
    pnode *makeFnode(pnode*, void*);
    pnode *makeUnode(pnode*, void*);
    pnode *makeSnode(void*);
    pnode *makeNnode(void*);

    char *userString(const char **s, bool in_source)
        {
            return (IP.getTranFunc(s, in_source));
        }
};

namespace {
    sOper ft_ops[] = { 
        sOper( TT_PLUS,  "+",  2, &sDataVec::v_plus ),
        sOper( TT_MINUS, "-",  2, &sDataVec::v_minus ),
        sOper( TT_TIMES, "*",  2, &sDataVec::v_times ),
        sOper( TT_MOD,   "%",  2, &sDataVec::v_mod ),
        sOper( TT_DIVIDE,"/",  2, &sDataVec::v_divide ),
        sOper( TT_COMMA, ",",  2, &sDataVec::v_comma ),
        sOper( TT_POWER, "^",  2, &sDataVec::v_power ),
        sOper( TT_EQ,    "=",  2, &sDataVec::v_eq ),
        sOper( TT_GT,    ">",  2, &sDataVec::v_gt ),
        sOper( TT_LT,    "<",  2, &sDataVec::v_lt ),
        sOper( TT_GE,    ">=", 2, &sDataVec::v_ge ),
        sOper( TT_LE,    "<=", 2, &sDataVec::v_le ),
        sOper( TT_NE,    "<>", 2, &sDataVec::v_ne ),
        sOper( TT_AND,   "&",  2, &sDataVec::v_and ),
        sOper( TT_OR,    "|",  2, &sDataVec::v_or ),
        sOper( TT_COLON, ":",  2, 0 ),
        sOper( TT_COND,  "?",  2, 0 ),
        sOper( TT_INDX,  "[",  2, 0 ),
        sOper( TT_RANGE, "[[", 2, 0 ),
        sOper( TT_END,   0,    0, 0 )
    };

    sFunc ft_uops[] = {
        sFunc( "-",             &sDataVec::v_uminus,    1 ),
        sFunc( "~",             &sDataVec::v_not,       1 ),
        sFunc( 0 )
    };

    sFunc ft_funcs[] = {
        sFunc( "mag",           &sDataVec::v_mag,       1 ),
        sFunc( "magnitude",     &sDataVec::v_mag,       1 ),
        sFunc( "ph",            &sDataVec::v_ph,        1 ),
        sFunc( "phase",         &sDataVec::v_ph,        1 ),
        sFunc( "j",             &sDataVec::v_j,         1 ),
        sFunc( "real",          &sDataVec::v_real,      1 ),
        sFunc( "re",            &sDataVec::v_real,      1 ),
        sFunc( "imag",          &sDataVec::v_imag,      1 ),
        sFunc( "im",            &sDataVec::v_imag,      1 ),
        sFunc( "db",            &sDataVec::v_db,        1 ),
        sFunc( "log10",         &sDataVec::v_log10,     1 ),
        sFunc( "log",           &sDataVec::v_log,       1 ),
        sFunc( "ln",            &sDataVec::v_ln,        1 ),
        sFunc( "exp",           &sDataVec::v_exp,       1 ),
        sFunc( "abs",           &sDataVec::v_mag,       1 ),
        sFunc( "sqrt",          &sDataVec::v_sqrt,      1 ),
        sFunc( "sin",           &sDataVec::v_sin,       1 ),
        sFunc( "cos",           &sDataVec::v_cos,       1 ),
        sFunc( "tan",           &sDataVec::v_tan,       1 ),
        sFunc( "asin",          &sDataVec::v_asin,      1 ),
        sFunc( "acos",          &sDataVec::v_acos,      1 ),
        sFunc( "atan",          &sDataVec::v_atan,      1 ),
        sFunc( "sinh",          &sDataVec::v_sinh,      1 ),
        sFunc( "cosh",          &sDataVec::v_cosh,      1 ),
        sFunc( "tanh",          &sDataVec::v_tanh,      1 ),
        sFunc( "asinh",         &sDataVec::v_asinh,     1 ),
        sFunc( "acosh",         &sDataVec::v_acosh,     1 ),
        sFunc( "atanh",         &sDataVec::v_atanh,     1 ),
        sFunc( "j0",            &sDataVec::v_j0,        1 ),
        sFunc( "j1",            &sDataVec::v_j1,        1 ),
        sFunc( "jn",            &sDataVec::v_jn,        1 ),
        sFunc( "y0",            &sDataVec::v_y0,        1 ),
        sFunc( "y1",            &sDataVec::v_y1,        1 ),
        sFunc( "yn",            &sDataVec::v_yn,        1 ),
        sFunc( "cbrt",          &sDataVec::v_cbrt,      1 ),
        sFunc( "erf",           &sDataVec::v_erf,       1 ),
        sFunc( "erfc",          &sDataVec::v_erfc,      1 ),
        sFunc( "gamma",         &sDataVec::v_gamma,     1 ),
        sFunc( "norm",          &sDataVec::v_norm,      1 ),
        sFunc( "fft",           &sDataVec::v_fft,       1 ),
        sFunc( "ifft",          &sDataVec::v_ifft,      1 ),
        sFunc( "pos",           &sDataVec::v_pos,       1 ),
        sFunc( "mean",          &sDataVec::v_mean,      1 ),
        sFunc( "vector",        &sDataVec::v_vector,    1 ),
        sFunc( "unitvec",       &sDataVec::v_unitvec,   1 ),
        sFunc( "length",        &sDataVec::v_size,      1 ),
        sFunc( "interpolate",   &sDataVec::v_interpolate, 1 ),
        sFunc( "deriv",         &sDataVec::v_deriv,     1 ),
        sFunc( "integ",         &sDataVec::v_integ,     1 ),
        sFunc( "rms",           &sDataVec::v_rms,       1 ),
        sFunc( "sum",           &sDataVec::v_sum,       1 ),
        sFunc( "floor",         &sDataVec::v_floor,     1 ),
        sFunc( "sgn",           &sDataVec::v_sgn,       1 ),
        sFunc( "ceil",          &sDataVec::v_ceil,      1 ),

        sFunc( "int",           &sDataVec::v_rint,      1 ),
        sFunc( "v" ),
        sFunc( "i" ),
        sFunc( "p"),

        // Stat funcs
        sFunc( "beta",          &sDataVec::v_beta,      1 ),
        sFunc( "binomial",      &sDataVec::v_binomial,  1 ),
        sFunc( "chisq",         &sDataVec::v_chisq,     1 ),
        sFunc( "erlang",        &sDataVec::v_erlang,    1 ),
        sFunc( "exponential",   &sDataVec::v_exponential, 1 ),
        sFunc( "ogauss",        &sDataVec::v_ogauss,    1 ),
        sFunc( "poisson",       &sDataVec::v_poisson,   1 ),
        sFunc( "rnd",           &sDataVec::v_rnd,       1 ),
        sFunc( "tdist",         &sDataVec::v_tdist,     1 ),

        // Measure system exports
        sFunc( "mmin",          &sDataVec::v_mmin,      3 ),
        sFunc( "mmax",          &sDataVec::v_mmax,      3 ),
        sFunc( "mpp",           &sDataVec::v_mpp,       3 ),
        sFunc( "mavg",          &sDataVec::v_mavg,      3 ),
        sFunc( "mrms",          &sDataVec::v_mrms,      3 ),
        sFunc( "mpw",           &sDataVec::v_mpw,       3 ),
        sFunc( "mrft",          &sDataVec::v_mrft,      3 ),

        // HSPICE funcs
        sFunc( "unif",          &sDataVec::v_hs_unif,   2 ),
        sFunc( "aunif",         &sDataVec::v_hs_aunif,  2 ),
        sFunc( "gauss",         &sDataVec::v_hs_gauss,  3 ),
        sFunc( "agauss",        &sDataVec::v_hs_agauss, 3 ),
        sFunc( "limit",         &sDataVec::v_hs_limit,  2 ),
        sFunc( "pow",           &sDataVec::v_hs_pow,    2 ),
        sFunc( "pwr",           &sDataVec::v_hs_pwr,    2 ),
        sFunc( "sign",          &sDataVec::v_hs_sign,   2 ),

        sFunc( 0 )
    };

    sHtab *funcname_tab;


    // Hash table for function names.
    //
    sFunc *find_func(const char *name)
    {
        if (!funcname_tab) {
            funcname_tab = new sHtab(sHtab::get_ciflag(CSE_FUNC));

            for (sFunc *f = ft_funcs;  f->name(); f++)
                funcname_tab->add(f->name(), f);
        }
        return ((sFunc*)sHtab::get(funcname_tab, name));
    }


    bool is_int(const char *buf)
    {
        for (const char *s = buf; *s; s++) {
            if (!isdigit(*s))
                return (false);
        }
        return (true);
    }
}


pnlist *
IFsimulator::GetPtree(wordlist *wl, bool check)
{
    if (!wl) {
        GRpkg::self()->ErrPrintf(ET_WARN, "null arithmetic expression.\n");
        return (0);
    }
    sCKT *ckt = ft_curckt ? ft_curckt->runckt() : 0;

    // Separate the parse nodes explicitly when double quoted
    pnlist *plend = 0, *pl0 = 0;
    char *xsbuf = 0;
    char buf[BSIZE_SP];
    for (wordlist *ww = wl; ww; ww = ww->wl_next) {
        if (*ww->wl_word == '"' &&
                *(ww->wl_word + strlen(ww->wl_word) - 1) == '"') {
            // A quoted word is taken as a stand-alone expression
            if (xsbuf) {
                pnlist *pl = GetPtree(xsbuf, check);
                if (pl) {
                    if (!pl0)
                        plend = pl0 = pl;
                    else {
                        while (plend->next())
                            plend = plend->next();
                        plend->set_next(pl);
                    }
                }
                else {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "evaluation failed: %s.\n", xsbuf);
                }
                delete [] xsbuf;
                xsbuf = 0;
            }
            strcpy(buf, ww->wl_word + 1);
            *(buf + strlen(buf) - 1) = 0;
            xsbuf = lstring::copy(buf);
            pnlist *pl = 0;
            if (OP.isVec(xsbuf, ckt)) {
                pnode *p = new pnode(xsbuf, xsbuf, 0);
                if (p)
                    pl = new pnlist(p, 0);
            }
            else
                pl = GetPtree(xsbuf, check);
            if (pl) {
                if (!pl0)
                    plend = pl0 = pl;
                else {
                    while (plend->next())
                        plend = plend->next();
                    plend->set_next(pl);
                }
            }
            else {
                GRpkg::self()->ErrPrintf(ET_ERROR, "evaluation failed: %s.\n",
                    xsbuf);
            }
            delete [] xsbuf;
            xsbuf = 0;
        }
        else {
            if (xsbuf) {
                buf[0] = ' ';
                strcpy(buf+1, ww->wl_word);
                xsbuf = lstring::build_str(xsbuf, buf);
            }
            else if (!ww->wl_next && !is_int(ww->wl_word) &&
                    OP.isVec(ww->wl_word, ckt)) {
                pnode *p = new pnode(0, ww->wl_word, 0);
                if (p) {
                    pnlist *pl = new pnlist(p, 0);
                    if (!pl0)
                        plend = pl0 = pl;
                    else {
                        while (plend->next())
                            plend = plend->next();
                        plend->set_next(pl);
                    }
                }
            }
            else
                xsbuf = lstring::copy(ww->wl_word);
        }
    }
    if (xsbuf) {
        pnlist *pl = GetPtree(xsbuf, check);
        if (pl) {
            if (!pl0)
                plend = pl0 = pl;
            else {
                while (plend->next())
                    plend = plend->next();
                plend->set_next(pl);
            }
        }
        else {
            GRpkg::self()->ErrPrintf(ET_ERROR, "evaluation failed: %s.\n",
                xsbuf);
        }
        delete [] xsbuf;
        xsbuf = 0;
    }
    return (pl0);
}


// Parse the string and return a node tree.  If check, check the tree
// for errors.
//
pnlist *
IFsimulator::GetPtree(const char *xsbuf, bool check)
{
    if (!xsbuf)
        return (0);
    if (*xsbuf == '\'')
        // skip delimiter
        xsbuf++;

    // instantiate the parser
    unsigned int flags =
        (PRSR_UNITS | PRSR_AMPHACK | PRSR_NODEHACK | PRSR_USRSTR);
    spElement elements[STACKSIZE];
    Parser P = Parser(elements, flags);
    P.init(xsbuf, 0);
    const char *thisone = xsbuf;

    pnlist *plend = 0, *pl0 = 0;
    while (*P.residue() != '\0') {
        pnode *p = P.parse();
        if (!p) {
            // parse error
            const char *errmsg = P.getErrMesg();
            GRpkg::self()->ErrPrintf(ET_ERROR, "parser returned error: %s.\n",
                errmsg);
            return (0);
        }
        if (check) {
            if (!p->checkvalid()) {
                delete p;
                return (0);
            }
            // Can't do this unless the tree is checked!
            p->collapse(&p);
        }

        // Now snag the name... Much trouble...
        while (isspace(*thisone))
            thisone++;
        char *s;
        char *bf = new char[P.residue() - thisone + 1];
        for (s = bf; thisone < P.residue(); s++, thisone++)
            *s = *thisone;
        do {s--;}
        while (isspace(*s) && s != bf);
        *(s+1) = '\0';
        s = bf;
        if (*s == '"')
            s++;
        char *t = s + strlen(s) - 1;
        if (*t == '"')
            *t = 0;
        p->set_name(s);
        delete [] bf;

        pnlist *pl = new pnlist(p, 0);
        if (!pl0)
            plend = pl0 = pl;
        else {
            while (plend->next())
                plend = plend->next();
            plend->set_next(pl);
        }
    }
    return (pl0);
}


// Parse the string for one node and update the string.  If param, we
// are expanding a .param expression.
//
pnode *
IFsimulator::GetPnode(const char **xsptr, bool check, bool param, bool quiet)
{
    if (!xsptr || !*xsptr)
        return (0);
    const char *xsbuf = *xsptr;
    if (*xsbuf == '\'')
        // skip delimiter
        xsbuf++;

    // instantiate the parser
    unsigned int flags = PRSR_UNITS | PRSR_AMPHACK | PRSR_NODEHACK;
    if (!param)
        flags |= PRSR_USRSTR;
    spElement elements[STACKSIZE];
    Parser P = Parser(elements, flags);
    P.init(xsbuf, 0);
    const char *thisone = xsbuf;

    pnode *p = P.parse();
    if (!p) {
        // parse error
        return (0);
    }
    if (check) {
        if (quiet) {
            if (!p->checkvalid_quiet()) {
                delete p;
                return (0);
            }
        }
        else {
            if (!p->checkvalid()) {
                delete p;
                return (0);
            }
        }
        // Can't do this unless the tree is checked!
        p->collapse(&p);
    }

    // Now snag the name... Much trouble...
    while (isspace(*thisone))
        thisone++;
    char *s;
    char *bf = new char[P.residue() - thisone + 1];
    for (s = bf; thisone < P.residue(); s++, thisone++)
        *s = *thisone;
    do {s--;}
    while (isspace(*s) && s != bf);
    *(s+1) = '\0';
    s = bf;
    if (*s == '"')
        s++;
    char *t = s + strlen(s) - 1;
    if (*t == '"')
        *t = 0;
    p->set_name(s);
    delete [] bf;

    *xsptr = P.residue();
    return (p);
}


// Return true if fname clashes with an internal function name.
//
bool
IFsimulator::CheckFuncName(const char* fname)
{
    sFunc *f = find_func(fname);
    if (f)
        return (true);
    if (
            (lstring::ciinstr("vi", *fname) &&
            (!*(fname+1) ||
            (!*(fname+2) && lstring::ciinstr("mpri", *(fname+1))) ||
            (!*(fname+3) && lstring::cieq(fname+1, "db")))))
        return (true);
    if (!fname[1] && (fname[0] == 'p' || fname[0] == 'P'))
        return (true);
    return (false);
}


// Given a pointer to an element, make a pnode out of it (if it already
// is one, return a pointer to it). If it isn't of type VALUE, then return
// 0.
//
pnode *
spElement::makeNode(void *uarg)
{
    if (token != TT_VALUE)
        return (0);
    if (type == DT_STRING) {
        pnode *p = makeSnode(uarg);
        p->set_type(PN_VEC);
        return (p);
    }
    if (type == DT_USTRING) {
        pnode *p = makeSnode(uarg);
        p->set_type(PN_TRAN);
        return (p);
    }
    if (type == DT_NUM)
        return (makeNnode(uarg));
    if (type == DT_PNODE)
        return (vu.node);
    return (0);
}


// Binop node
//
pnode *
spElement::makeBnode(pnode *arg1, pnode *arg2, void*)
{
    sOper *o;
    for (o = ft_ops; o->name(); o++) {
        if (o->optype() == token)
            break;
    }
    if (!o->name()) {
        GRpkg::self()->ErrPrintf(ET_INTERR, "makeBnode: no such op num %d.\n",
            token);
        return (0);
    }
    if (token == TT_COND) {
        if (!arg2 || !arg2->oper() || arg2->oper()->optype() != TT_COLON) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "'?' conditional syntax error.\n");
            return (0);
        }
    }
    return (new pnode(o, arg1, arg2));
}


// Unop node
//
pnode *
spElement::makeUnode(pnode *arg, void*)
{
    pnode *p;
    if (token == TT_UMINUS)
        p = new pnode(ft_uops, arg);
    else if (token == TT_NOT)
        p = new pnode(ft_uops+1, arg);
    else {
        GRpkg::self()->ErrPrintf(ET_INTERR, "makeUnode: no such op num %d.\n",
            token);
        return (0);
    }
    return (p);
}


// Function node. Something like f(a) could be three things -- a call to a
// standard function, a vector name, or a user-defined function.
//
pnode *
spElement::makeFnode(pnode *arg, void*)
{
    char *string = lstring::copy(vu.string);
    for (char *s = string; *s; s++) {
        if (isupper(*s))
            *s = tolower(*s);
    }

    // Handle v() and i() here.
    if (*string == 'v' || *string == 'i') {
        pnode *p = arg->vecstring(string);
        if (p != 0) {
            delete [] string;
            delete [] vu.string;
            vu.string = 0;
            return (p);
        }
    }
    if (*string == 'p' && *(string+1) == 0) {
        pnode *p = arg->vecstring(string);
        if (p != 0) {
            delete [] string;
            delete [] vu.string;
            vu.string = 0;
            return (p);
        }
    }

    // Put back original case.
    strcpy(string, vu.string);
    delete [] vu.string;
    vu.string = 0;

    sFunc *f = find_func(string);
    if (f) {
        delete [] string;
        return (new pnode(f, arg));
    }

    // Give the user-defined functions a try

    // The previous logic was to resolve macros at this point,
    // where they would be stitched into the curent tree being
    // built.
    /*
    p = GetUserFuncTree(string, arg);
    if (p) {
        delete arg;
        delete [] string;
        return (p);
    }
    */

    // The present logic defers resolving until run-time.  This
    // preserves the original call in user defined functions, and
    // keeps the text length reasonable.  The text length can grow
    // quite long if all macros are "flattened".

    // Note that we do the same thing below for functions that are
    // unresolved at this point.  They can be subsequently
    // defined, before the present macro is executed.

    if (Sp.IsUserFunc(string, arg)) {
        f = new sFunc(string, &sDataVec::v_undefined, 0);
        return (new pnode(f, arg, true));
    }

    // Kludge -- maybe it is really a variable name.
    if (arg && (arg->value() || arg->token_string())) {
        sLstr lstr;
        lstr.add(string);
        lstr.add_c('(');
        if (arg->value())
            lstr.add(arg->value()->name());
        else
            lstr.add(arg->token_string());
        lstr.add_c(')');
        sCKT *ckt = Sp.CurCircuit() ? Sp.CurCircuit()->runckt() : 0;
        if (OP.isVec(lstr.string(), ckt)) {
            delete arg;
            delete [] string;
            return (new pnode(0, lstr.string(), 0));
        }
    }

    if (Sp.GetFlag(FT_DEFERFN)) {
        // Add a dummy function call.  This will attempt to
        // resolve as a user-defined function at exec time.  This
        // is done only when creating text for a user-defined
        // function.

        f = new sFunc(string, &sDataVec::v_undefined, 0);
        return (new pnode(f, arg, true));
    }

    // Well, too bad
    GRpkg::self()->ErrPrintf(ET_ERROR, "unknown function %s.\n", string);
    delete [] string;
    return (0);
}


namespace {
    char *printG(double d)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%G", d);
        return (lstring::copy(buf));
    }
}


// Number node
//
pnode *
spElement::makeNnode(void*)
{
    char *s = printG(vu.real);
    sDataVec *v = new sDataVec(s, 0, 1, units);  // s saved
    delete units;
    units = 0;
    v->set_realval(0, vu.real);
    v->newtemp();

    return (new pnode(0, s, v));  // s copied
}


// String node
//
pnode *
spElement::makeSnode(void*)
{
    pnode *p = new pnode(0, vu.string, 0);
    delete [] vu.string;
    vu.string = 0;
    return (p);
}
// end of spElement functions


pnode::~pnode()
{
    delete [] pn_name;
    delete [] pn_string;
    delete pn_left;
    delete pn_right;
    if (pn_localval) {
        if (pn_value)
            delete pn_value;
        else if (pn_func) {
            delete [] pn_func->name();
            delete pn_func;
        }
    }
}


// See if there are any variables around which have length 0 and are
// not named 'list'.  The vectors are not normally accessed until
// ft_evaluate() is called.
//
bool
pnode::checkvalid() const
{
    if (pn_string) {
        if (!pn_value && pn_type == PN_VEC) {
            sCKT *ckt = Sp.CurCircuit() ? Sp.CurCircuit()->runckt() : 0;
            sDataVec *d = OP.vecGet(pn_string, ckt);
            if (!d || (d->length() == 0 && !lstring::eq(d->name(), "list"))) {
                if (lstring::eq(pn_string, "all")) {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "no matching vectors.\n", pn_string);
                }
                else
                    Sp.Error(E_NOVEC, 0, pn_string);
                return (false);
            }
        }
    }
    else if (pn_func) {
        if (pn_left && !pn_left->checkvalid())
            return (false);
    }
    else if (pn_op) {
        if (pn_left && !pn_left->checkvalid())
            return (false);
        if (pn_right && !pn_right->checkvalid())
            return (false);
    }
    else {
        GRpkg::self()->ErrPrintf(ET_INTERR, "checkvalid: bad node.\n");
        return (false);
    }
    return (true);
}


// Silent version.
//
bool
pnode::checkvalid_quiet() const
{
    if (pn_string) {
        if (!pn_value && pn_type == PN_VEC) {
            sCKT *ckt = Sp.CurCircuit() ? Sp.CurCircuit()->runckt() : 0;
            sDataVec *d = OP.vecGet(pn_string, ckt);
            if (!d || (d->length() == 0 && !lstring::eq(d->name(), "list")))
                return (false);
        }
    }
    else if (pn_func) {
        if (pn_left && !pn_left->checkvalid())
            return (false);
    }
    else if (pn_op) {
        if (pn_left && !pn_left->checkvalid())
            return (false);
        if (pn_right && !pn_right->checkvalid())
            return (false);
    }
    else
        return (false);
    return (true);
}


// Return true is the expression contains numeric values only.
//
bool
pnode::is_constant() const
{
    if (pn_string) {
        if (!is_numeric())
            return (false);
    }
    else if (pn_func) {
        if (!pn_func->func()) {
            return (false);
        }
        else if (pn_func->func() == &sDataVec::v_undefined) {
            // Descend into the macros, too.
            pnode *p = Sp.GetUserFuncTree(pn_func->name(), pn_left);
            if (p) {
                bool r = p->is_constant();
                delete p;
                if (!r)
                    return (false);
            }
        }
        if (pn_left) {
            if (!pn_left->is_constant())
                return (false);
        }
    }
    else if (pn_op) {
        if (pn_left) {
            if (!pn_left->is_constant())
                return (false);
        }
        if (pn_right) {
            if (!pn_right->is_constant())
                return (false);
        }
    }
    else {
        return (false);
    }
    return (true);
}


// Special silent tree-checking function.  Return values are
// 0  No errors.
// 1  Tree contains vector reference(s).  These are not handled
//    typically in parameter expressions.  Also return 1 if the
//    tree calls a tran func.
// 2  Serious error.
//
int
pnode::checktree() const
{
    int ret = 0;
    if (pn_string) {
        if (!pn_value && pn_type == PN_VEC) {
            // Found a vector reference, we allow any length=1 vector.
            sDataVec *dv = OP.vecGet(pn_string, 0, true);
            if (!dv || dv->length() != 1) {
                if (!ret)
                    ret = 1;
            }
        }
        else if (pn_type == PN_TRAN) {
            if (!ret)
                ret = 1;
        }
    }
    else if (pn_func) {
        if (!pn_func->func()) {
            const char *n = pn_func->name();
            if (!n[1] && (n[0] == 'v' || n[0] == 'i' || n[0] == 'p')) {
                // Found a v(), i(), or p() form.
                if (!ret)
                    ret = 1;
            }
        }
        else if (pn_func->func() == &sDataVec::v_undefined) {
            // Descend into the macros, too.
            pnode *p = Sp.GetUserFuncTree(pn_func->name(), pn_left);
            if (p) {
                int r = p->checktree();
                if (ret < r)
                    ret = r;
                delete p;
            }
        }
        if (pn_left) {
            int r = pn_left->checktree();
            if (ret < r)
                ret = r;
        }
    }
    else if (pn_op) {
        if (pn_left) {
            int r = pn_left->checktree();
            if (ret < r)
                ret = r;
        }
        if (pn_right) {
            int r = pn_right->checktree();
            if (ret < r)
                ret = r;
        }
    }
    else {
        // error: bad node
        ret = 2;
    }
    return (ret);
}


// Evaluate and simplify constant parts.  The argument should be the
// address of this.
//
void
pnode::collapse(pnode **pp)
{
    if (pn_left)
        pn_left->collapse(&pn_left);
    if (pn_right)
        pn_right->collapse(&pn_right);
    if (pn_string)
        return;
    if (pn_func) {
        if (pn_func == ft_uops || pn_func == ft_uops+1) {
            // Unary minus or NOT operator.
            if (pn_left->is_numeric()) {
                sDataVec *v = Sp.Evaluate(this);
                if (v) {
                    pn_value = v;
                    pn_string = printG(v->realval(0));
                    delete pn_left;
                    pn_left = 0;
                    pn_func = 0;
                }
                return;
            }
        }

        if (pn_func->func()) {
            bool doit = !pn_left || pn_left->is_numeric();
            if (!doit && pn_left->optype() == TT_COMMA) {
                pnode *px = pn_left;
                doit = true;
                while (px) {
                    if (!px->pn_left->is_numeric()) {
                        doit = false;
                        break;
                    }
                    if (px->pn_right->is_numeric())
                       break;
                    if (px->pn_right->optype() == TT_COMMA) {
                        px = px->pn_right;
                        continue;
                    }
                    // bad argument list
                    doit = false;
                    break;
                }
            }
            if (doit) {
                sDataVec *v = Sp.Evaluate(this);
                if (v) {
                    pn_value = v;
                    pn_string = printG(v->realval(0));
                    delete pn_left;
                    pn_left = 0;
                    if (pn_localval) {
                        delete pn_func;
                        pn_localval = 0;
                    }
                    pn_func = 0;
                }
            }
        }
    }
    else if (pn_op) {
        if (pn_op->optype() == TT_COMMA)
            return;
        if (pn_op->optype() == TT_COLON)
            return;
        if (pn_left && pn_right) {
            if (pn_left->is_numeric() && pn_right->is_numeric()) {
                sDataVec *v = Sp.Evaluate(this);
                if (v) {
                    pn_value = v;
                    pn_string = printG(v->realval(0));
                    delete pn_left;
                    pn_left = 0;
                    delete pn_right;
                    pn_right = 0;
                    pn_op = 0;
                }
                return;
            }
            if (pn_op->optype() == TT_PLUS) {
                if (pn_left->is_const_zero()) {
                    *pp = pn_right;
                    pn_right = 0;
                    delete this;
                }
                else if (pn_right->is_const_zero()) {
                    *pp = pn_left;
                    pn_left = 0;
                    delete this;
                }
            }
            else if (pn_op->optype() == TT_MINUS) {
                if (pn_left->is_const_zero()) {
                    pn_func = ft_uops;
                    pn_op = 0;
                    delete pn_left;
                    pn_left = pn_right;
                    pn_right = 0;
                }
                else if (pn_right->is_const_zero()) {
                    *pp = pn_left;
                    pn_left = 0;
                    delete this;
                }
            }
            else if (pn_op->optype() == TT_TIMES) {
                if (pn_left->is_const_one()) {
                    *pp = pn_right;
                    pn_right = 0;
                    delete this;
                }
                else if (pn_left->is_const_zero()) {
                    *pp = pn_left;
                    pn_left = 0;
                    delete this;
                }
                else if (pn_right->is_const_one()) {
                    *pp = pn_left;
                    pn_left = 0;
                    delete this;
                }
                else if (pn_right->is_const_zero()) {
                    *pp = pn_right;
                    pn_right = 0;
                    delete this;
                }
            }
            else if (pn_op->optype() == TT_DIVIDE) {
                if (pn_right->is_const_one()) {
                    *pp = pn_left;
                    pn_left = 0;
                    delete this;
                }
                else if (pn_left->is_const_zero()) {
                    *pp = pn_left;
                    pn_left = 0;
                    delete this;
                }
            }
        }
    }
}


// Stitch in the user-defined functions, replacing the calls.
// This function is not used.
//
pnode *
pnode::expand_macros()
{
    if (pn_func) {
        pn_left = pn_left->expand_macros();
        if (pn_func->func() == &sDataVec::v_undefined) {
            pnode *p = Sp.GetUserFuncTree(pn_func->name(), pn_left);
            if (p)
                return (p);
        }
    }
    else if (pn_op) {
        pn_left = pn_left->expand_macros();
        if (pn_right)
            pn_right = pn_right->expand_macros();
    }
    return (this);
}


// Search the tree for function references to transient UDFs, i.e.,
// those defined within a subcircuit.  Promote these, after parameter
// expanding the body text, to new UDFs defined in the current cell,
// with a new unique function name.  The MapTab ensures that we do the
// hard work once only for each macro (arg is 0 at top level).
//
void
pnode::promote_macros(const sParamTab *ptab, sMacroMapTab *mtab)
{
    sMacroMapTab *mytab = 0;
    if (pn_func) {
        if (!mtab) {
            mytab = new sMacroMapTab;
            mtab = mytab;
        }
        if (pn_func->func() == &sDataVec::v_undefined)
            Sp.TestAndPromote(this, ptab, mtab);
        pn_left->promote_macros(ptab, mtab);
    }
    else if (pn_op) {
        if (!mtab) {
            mytab = new sMacroMapTab;
            mtab = mytab;
        }
        pn_left->promote_macros(ptab, mtab);
        if (pn_right)
            pn_right->promote_macros(ptab, mtab);
    }
    delete mytab;
}


namespace {
    inline bool is_gnd(const char *str)
    {
        return (str[0] == '0' && !str[1]);
    }
}


// Special handling for vector references:
// v(), vm(), vp(), vr(), vi() vdb() ( each (x) or (x,y) )
// i(), img(), ip(), ir(), ii() idb()
// p()
//
pnode *
pnode::vecstring(const char *fnstr)
{
    if (*fnstr == 'v') {

        sFunc *f;
        if (!*(fnstr+1))
            f = 0;
        else if (*(fnstr+1) == 'm' && *(fnstr+2) == '\0')
            f = find_func("mag");
        else if (*(fnstr+1) == 'p' && *(fnstr+2) == '\0')
            f = find_func("ph");
        else if (*(fnstr+1) == 'r' && *(fnstr+2) == '\0')
            f = find_func("re");
        else if (*(fnstr+1) == 'i' && *(fnstr+2) == '\0')
            f = find_func("im");
        else if (*(fnstr+1) == 'd' && *(fnstr+2) == 'b' && *(fnstr+3) == '\0')
            f = find_func("db");
        else
            return (0);

        int arity = 0;
        const pnode *tp = this;
        if (tp)
            arity = 1;  
        for ( ; tp && tp->pn_op && (tp->pn_op->optype() == TT_COMMA);
                tp = tp->pn_right)
            arity++;
        if (arity < 1 || arity > 2)
            return (0);

        pnode *p = 0;
        // Avoid trying to resolve "v(0)" which will fail.
        if (arity == 1) {
            if (!pn_string)
                return (0);

            if (is_gnd(pn_string)) {
                sDataVec *v = new sDataVec(lstring::copy("0"), 0, 1, 0);
                v->set_realval(0, 0.0);
                v->newtemp();
                p = new pnode(0, "0", v);
                delete this;
            }
            else
                p = new pnode(find_func("v"), this);
        }
        else {
            if (!pn_left->pn_string || !pn_right->pn_string)
                return (0);

            if (is_gnd(pn_right->pn_string)) {
                if (is_gnd(pn_left->pn_string)) {
                    // v(0, 0)
                    sDataVec *v = new sDataVec(lstring::copy("0"), 0, 1, 0);
                    v->set_realval(0, 0.0);
                    v->newtemp();
                    p = new pnode(0, "0", v);
                    delete this;
                }
                else {
                    // v(x, 0)
                    p = new pnode(find_func("v"), pn_left);
                    pn_left = 0;
                    delete this;
                }
            }
            else if (is_gnd(pn_left->pn_string)) {
                // v(0, x)
                pnode *p2 = new pnode(find_func("v"), pn_right);
                pn_right = 0;
                delete this;
                spElement e;
                e.token = TT_UMINUS;
                p = e.makeUnode(p2, 0);
            }
            else {
                pnode *p1 = new pnode(find_func("v"), pn_left);
                pn_left = 0;
                pnode *p2 = new pnode(find_func("v"), pn_right);
                pn_right = 0;
                delete this;
                spElement e;
                e.token = TT_MINUS;
                p = e.makeBnode(p1, p2, 0);
            }
        }
        if (f)
            return (new pnode(f, p));
        return (p);
    }
    if (*fnstr == 'i') {
        const pnode *thispn = this;
        if (!thispn || !pn_string)
            return (0);

        sFunc *f;
        if (!*(fnstr+1))
            f = 0;
        // Avoid clash with im() vector function.
        else if (*(fnstr+1) == 'm' && *(fnstr+2) == 'g' && *(fnstr+3) == '\0')
            f = find_func("mag");
        else if (*(fnstr+1) == 'p' && *(fnstr+2) == '\0')
            f = find_func("ph");
        else if (*(fnstr+1) == 'r' && *(fnstr+2) == '\0')
            f = find_func("re");
        else if (*(fnstr+1) == 'i' && *(fnstr+2) == '\0')
            f = find_func("im");
        else if (*(fnstr+1) == 'd' && *(fnstr+2) == 'b' && *(fnstr+3) == '\0')
            f = find_func("db");
        else
            return (0);

        char *t = strchr(pn_string, '#');
        if (!t || !lstring::eq(t+1, "branch")) {
            t = new char[strlen(pn_string) + 8];
            strcpy(t, pn_string);
            strcat(t, "#branch");
            delete [] pn_string;
            pn_string = t;
        }
        pnode *p = new pnode(find_func("i"), this);
        if (f)
            return (new pnode(f, p));
        return (p);
    }
    if (lstring::eq(fnstr, "p")) {
        // Replace "p(source)" with "@source[p]".
        const pnode *thispn = this;
        if (!thispn || !pn_string)
            return (0);
        char *t = new char[strlen(pn_string) + 8];
        t[0] = '@';
        strcpy(t+1, pn_string);
        strcat(t, "[p]");
        delete [] pn_string;
        pn_string = t;

        return (new pnode(find_func("p"), this));
    }
    return (0);
}


// Create the macro expression in lstr.  The operator precedence table
// from the parser is used to figure out when to add parentheses.
//
void
pnode::get_string(sLstr &lstr, TokenType parent_optype, bool rhs) const
{
    const pnode *thispn = this;
    if (!thispn) {
        lstr.add("<undefined>");
        return;
    }
    if (pn_string)
        lstr.add(pn_string);
    else if (pn_func) {
        if (pn_func == ft_uops) {
            // Unary minus;
            lstr.add("-");
            pn_left->get_string(lstr, TT_UMINUS);
            return;
        }
        if (pn_func == ft_uops+1) {
            // Not.
            lstr.add("~");
            pn_left->get_string(lstr, TT_NOT);
            return;
        }
        lstr.add(pn_func->name());
        lstr.add_c('(');
        pn_left->get_string(lstr);
        lstr.add_c(')');
    }
    else if (pn_op) {
        bool prn = Parser::parenTable(optype(), parent_optype, rhs);
        if (prn)
            lstr.add_c('(');
        pn_left->get_string(lstr, optype(), false);
        lstr.add(pn_op->name());
        pn_right->get_string(lstr, optype(), true);
        if (prn)
            lstr.add_c(')');
    }
    else
        lstr.add("<unknown>");
}


// Return the expression string, single-quoted if the argument is true.
//
char *
pnode::get_string(bool squote) const
{
    sLstr lstr;
    if (squote)
        lstr.add_c('\'');
    get_string(lstr);
    if (squote)
        lstr.add_c('\'');
    return (lstr.string_trim());
}


namespace {
    // Find the n'th arg in the arglist, returning 0 if there isn't one. 
    // Since comma has such a low priority and associates to the right, we
    // can just follow the right branch of the tree num times.  Note that
    // we start at 1 when numbering the args.
    //
    const pnode *ntharg(int num, const pnode *args)
    {
        const pnode *ptry = args;
        if (num > 1) {
            while (--num > 0) {
                if (ptry && ptry->oper() &&
                        (ptry->oper()->optype() != TT_COMMA)) {
                    if (num == 1)
                        break;
                    else
                        return (0);
                }
                ptry = ptry->right();
            }
        }
        if (ptry && ptry->oper() && (ptry->oper()->optype() == TT_COMMA))
            ptry = ptry->left();
        return (ptry);
    }
}


// Copy the tree and optionally replace formal args with the right
// stuff if these are not null.  In this case, the args are the formal
// argument names, separated by null bytes, and nn is the replacement
// TT_COMMA argument list.
//
pnode *
pnode::copy(const char *args, const pnode *nn) const
{
    const pnode *thispn = this;
    if (!thispn)
        return (0);
    if (pn_string && !pn_value) {
        if (args) {
            const char *s = args;
            int i = 1;
            while (*s) {
                if (lstring::eq(s, pn_string))
                    // a formal parameter
                    return (ntharg(i, nn)->copy());
                i++;
                while (*s++) ;   // Get past the last '\0'.
            }
        }
        pnode *px = new pnode(0, pn_string, 0);
        px->pn_type = pn_type;
        return (px);
    }
    if (pn_value) {
        if (pn_localval) {
            sDataVec *v = pn_value->copy();
            return (new pnode(0, pn_string, v, true));
        }
        return (new pnode(0, pn_string, pn_value));
    }
    if (pn_func) {
        if (pn_localval) {
            sFunc *f = new sFunc(lstring::copy(pn_func->name()),
                pn_func->func(), pn_func->argc());
            return (new pnode(f, pn_left->copy(args, nn), true));
        }
        return (new pnode(pn_func, pn_left->copy(args, nn)));
    }
    if (pn_op)
        return (new pnode(pn_op, pn_left->copy(args, nn),
            (pn_op->argc() == 2 ? pn_right->copy(args, nn) : 0)));

    GRpkg::self()->ErrPrintf(ET_INTERR, "copy: bad parse node.\n");
    return (0);
}


// Make a local copy of all vectors, so that they won't be freed
// in GC.  Used in UDF trees, which are persistent.
//
void
pnode::copyvecs()
{
    const pnode *thispn = this;
    if (!thispn)
        return;
    if (pn_value) {
        // We specifically don't add this to the plot list
        // so it won't get gc'ed.
        //
        pn_value = pn_value->copy();
        pn_value->set_scale(0);
        pn_value->set_plot(0);
        pn_localval = true;
    }
    else if (pn_op) {
        pn_left->copyvecs();
        if (pn_right)
            pn_right->copyvecs();
    }
    else if (pn_func)
        pn_left->copyvecs();
}

