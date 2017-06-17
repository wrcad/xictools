
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
 $Id: sced_expr.cc,v 5.14 2015/06/11 01:12:30 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "spnumber.h"


//#define DEBUG

// This implements a "bare bones" WRspice expression parser, used to
// identify expressions in a plot command line.  Identifying
// expressions provides the ability to color the node/branch plot
// marks in the schematic with the same color as the plot trace where
// they are used.

// Define this for a real expression parser/evaluator.
//
#define WITH_EVAL

namespace sced_expr {
    struct pnode_t;
    typedef double(PNfunc)(pnode_t*);

    // The parser requires some kind of a parse node, though we really
    // only care about the lexical analysis.  These will be assembled into
    // a tree by the parser.
    //
    struct pnode_t
    {
        pnode_t(pnode_t *a, pnode_t *b)
            {
                p_left = a;
                p_right = b;
#ifdef WITH_EVAL
                p_val = 0.0;
                p_string = 0;
                p_eval = 0;
#endif
            }

        ~pnode_t()
            {
                delete p_left;
                delete p_right;
#ifdef WITH_EVAL
                delete [] p_string;
#endif
            }

        pnode_t *p_left;
        pnode_t *p_right;
#ifdef WITH_EVAL
        double p_val;
        char *p_string;
        PNfunc *p_eval;
#endif
    };
}

using namespace sced_expr;

typedef pnode_t ParseNode;
#include "spparse.h"

namespace {
#ifdef WITH_EVAL
    PNfunc v_constant;
    PNfunc v_variable;

    // Operations. These should really be considered functions.
    //
    struct sOper
    {
        sOper(TokenType n, const char *na, int ac, PNfunc *f)
            {
                op_num = n;
                op_name = na;
                op_arity = ac;
                op_func = f;
            }
            
        TokenType optype()      { return (op_num); }
        const char *name()      { return (op_name); }
        int argc()              { return (op_arity); }
        PNfunc *func()          { return (op_func); }

    private:
        TokenType op_num;       // From parser #defines.
        const char *op_name;    // Printing name.
        char op_arity;          // One or two.
        PNfunc *op_func;        // Evaluation method.
    };

    PNfunc v_plus;
    PNfunc v_minus;
    PNfunc v_times;
    PNfunc v_mod;
    PNfunc v_divide;
    PNfunc v_comma;
    PNfunc v_power;
    PNfunc v_eq;
    PNfunc v_gt;
    PNfunc v_lt;
    PNfunc v_ge;
    PNfunc v_le;
    PNfunc v_ne;
    PNfunc v_and;
    PNfunc v_or;
    PNfunc v_cond;

    PNfunc v_uminus;
    PNfunc v_not;

    sOper ops[] = {
        sOper( TT_PLUS,  "+",  2, &v_plus ),
        sOper( TT_MINUS, "-",  2, &v_minus ),
        sOper( TT_TIMES, "*",  2, &v_times ),
        sOper( TT_MOD,   "%",  2, &v_mod ),
        sOper( TT_DIVIDE,"/",  2, &v_divide ),
        sOper( TT_COMMA, ",",  2, &v_comma ),
        sOper( TT_POWER, "^",  2, &v_power ),
        sOper( TT_EQ,    "=",  2, &v_eq ),
        sOper( TT_GT,    ">",  2, &v_gt ),
        sOper( TT_LT,    "<",  2, &v_lt ),
        sOper( TT_GE,    ">=", 2, &v_ge ),
        sOper( TT_LE,    "<=", 2, &v_le ),
        sOper( TT_NE,    "<>", 2, &v_ne ),
        sOper( TT_AND,   "&",  2, &v_and ),
        sOper( TT_OR,    "|",  2, &v_or ),
        sOper( TT_COLON, ":",  2, 0 ),
        sOper( TT_COND,  "?",  2, &v_cond ),
        sOper( TT_INDX,  "[",  2, 0 ),
        sOper( TT_RANGE, "[[", 2, 0 ),
        sOper( TT_END,   0,    0, 0 )
    };
    sOper uops[] = {
        sOper( TT_UMINUS,"-",  1, &v_uminus ),
        sOper( TT_NOT,   "~",  1, &v_not ),
        sOper( TT_END,     0,  0, 0 )
    };

    // The functions that are available.
    //
    struct sFunc
    {
        sFunc(const char *na, PNfunc *f, int ac)
            {
                fu_name = na;
                fu_func = f;
                fu_nargs = ac;
            }

        const char *name()      { return (fu_name); }
        void set_name(const char *n) { fu_name = n; }
        PNfunc *func()          { return (fu_func); }
        int argc()              { return (fu_nargs); }

    private:
        const char *fu_name;    // The print name of the function.
        PNfunc *fu_func;        // The evaluation method.
        int fu_nargs;           // Argument count.
    };

    PNfunc v_mag;
    PNfunc v_ph;
    PNfunc v_j;
    PNfunc v_real;
    PNfunc v_imag;
    PNfunc v_db;
    PNfunc v_log10;
    PNfunc v_log;
    PNfunc v_ln;
    PNfunc v_exp;
    PNfunc v_sqrt;
    PNfunc v_sin;
    PNfunc v_cos;
    PNfunc v_tan;
    PNfunc v_asin;
    PNfunc v_acos;
    PNfunc v_atan;
    PNfunc v_sinh;
    PNfunc v_cosh;
    PNfunc v_tanh;
    PNfunc v_asinh;
    PNfunc v_acosh;
    PNfunc v_atanh;
    PNfunc v_j0;
    PNfunc v_j1;
    PNfunc v_jn;
    PNfunc v_y0;
    PNfunc v_y1;
    PNfunc v_yn;
    PNfunc v_cbrt;
    PNfunc v_erf;
    PNfunc v_erfc;
    PNfunc v_gamma;
    PNfunc v_norm;
    PNfunc v_rnd;
    PNfunc v_ogauss;
    PNfunc v_fft;
    PNfunc v_ifft;
    PNfunc v_pos;
    PNfunc v_mean;
    PNfunc v_vector;
    PNfunc v_unitvec;
    PNfunc v_size;
    PNfunc v_interpolate;
    PNfunc v_deriv;
    PNfunc v_integ;
    PNfunc v_rms;
    PNfunc v_sum;
    PNfunc v_floor;
    PNfunc v_sgn;
    PNfunc v_ceil;
    PNfunc v_rint;
    PNfunc v_hs_unif;
    PNfunc v_hs_aunif;
    PNfunc v_hs_gauss;
    PNfunc v_hs_agauss;
    PNfunc v_hs_limit;
    PNfunc v_hs_pow;
    PNfunc v_hs_pwr;
    PNfunc v_hs_sign;

    sFunc funcs[] = {
        sFunc( "mag",         &v_mag,      1 ),
        sFunc( "magnitude",   &v_mag,      1 ),
        sFunc( "ph",          &v_ph,       1 ),
        sFunc( "phase",       &v_ph,       1 ),
        sFunc( "j",           &v_j,        1 ),
        sFunc( "real",        &v_real,     1 ),
        sFunc( "re",          &v_real,     1 ),
        sFunc( "imag",        &v_imag,     1 ),
        sFunc( "im",          &v_imag,     1 ),
        sFunc( "db",          &v_db,       1 ),
        sFunc( "log10",       &v_log10,    1 ),
        sFunc( "log",         &v_log,      1 ),
        sFunc( "ln",          &v_ln,       1 ),
        sFunc( "exp",         &v_exp,      1 ),
        sFunc( "abs",         &v_mag,      1 ),
        sFunc( "sqrt",        &v_sqrt,     1 ),
        sFunc( "sin",         &v_sin,      1 ),
        sFunc( "cos",         &v_cos,      1 ),
        sFunc( "tan",         &v_tan,      1 ),
        sFunc( "asin",        &v_asin,     1 ),
        sFunc( "acos",        &v_acos,     1 ),
        sFunc( "atan",        &v_atan,     1 ),
        sFunc( "sinh",        &v_sinh,     1 ),
        sFunc( "cosh",        &v_cosh,     1 ),
        sFunc( "tanh",        &v_tanh,     1 ),
        sFunc( "asinh",       &v_asinh,    1 ),
        sFunc( "acosh",       &v_acosh,    1 ),
        sFunc( "atanh",       &v_atanh,    1 ),
        sFunc( "j0",          &v_j0,       1 ),
        sFunc( "j1",          &v_j1,       1 ),
        sFunc( "jn",          &v_jn,       1 ),
        sFunc( "y0",          &v_y0,       1 ),
        sFunc( "y1",          &v_y1,       1 ),
        sFunc( "yn",          &v_yn,       1 ),
        sFunc( "cbrt",        &v_cbrt,     1 ),
        sFunc( "erf",         &v_erf,      1 ),
        sFunc( "erfc",        &v_erfc,     1 ),
        sFunc( "gamma",       &v_gamma,    1 ),
        sFunc( "norm",        &v_norm,     1 ),
        sFunc( "rnd",         &v_rnd,      1 ),
        sFunc( "ogauss",      &v_ogauss,   1 ),
        sFunc( "fft",         &v_fft,      1 ),
        sFunc( "ifft",        &v_ifft,     1 ),
        sFunc( "pos",         &v_pos,      1 ),
        sFunc( "mean",        &v_mean,     1 ),
        sFunc( "vector",      &v_vector,   1 ),
        sFunc( "unitvec",     &v_unitvec,  1 ),
        sFunc( "length",      &v_size,     1 ),
        sFunc( "interpolate", &v_interpolate, 1 ),
        sFunc( "deriv",       &v_deriv,    1 ),
        sFunc( "integ",       &v_integ,    1 ),
        sFunc( "rms",         &v_rms,      1 ),
        sFunc( "sum",         &v_sum,      1 ),
        sFunc( "floor",       &v_floor,    1 ),
        sFunc( "sgn",         &v_sgn,      1 ),
        sFunc( "ceil",        &v_ceil,     1 ),
        sFunc( "int",         &v_rint,     1 ),
        sFunc( "v",           0, 0 ),
        sFunc( "i",           0, 0 ),
        sFunc( "p",           0, 0 ),
        sFunc( "unif",        &v_hs_unif,   2 ),
        sFunc( "aunif",       &v_hs_aunif,  2 ),
        sFunc( "gauss",       &v_hs_gauss,  3 ),
        sFunc( "agauss",      &v_hs_agauss, 3 ),
        sFunc( "limit",       &v_hs_limit,  2 ),
        sFunc( "pow",         &v_hs_pow,    2 ),
        sFunc( "pwr",         &v_hs_pwr,    2 ),
        sFunc( "sign",        &v_hs_sign,   2 ),
        sFunc( 0,             0, 0 )
    };
#endif

    struct elem_t : public Element
    {
        pnode_t *makeNode(void*);
        pnode_t *makeBnode(pnode_t*, pnode_t*, void*);
        pnode_t *makeFnode(pnode_t*, void*);
        pnode_t *makeUnode(pnode_t*, void*);
        pnode_t *makeSnode(void*);
        pnode_t *makeNnode(void*);
        char *userString(const char**, bool);
    };


    pnode_t *
    elem_t::makeNode(void*)
    {
        if (token != TT_VALUE)
            return (0);
        if (type == DT_STRING) {
            pnode_t *p = makeSnode(0);
            return (p);
        }
        if (type == DT_USTRING) {
            pnode_t *p = makeSnode(0);
            return (p);
        }
        if (type == DT_NUM)
            return (makeNnode(0));
        if (type == DT_PNODE)
            return (vu.node);
        return (0);
    }


    pnode_t *
    elem_t::makeBnode(pnode_t *a, pnode_t *b, void*)
    {
        pnode_t *p = new pnode_t(a, b);
#ifdef WITH_EVAL
        for (int i = 0; ops[i].name(); i++) {
            if (ops[i].optype() == token) {
                p->p_eval = ops[i].func();
                break;
            }
        }
#endif
        return (p);
    }


    pnode_t *
    elem_t::makeFnode(pnode_t *a, void*)
    {
        pnode_t *p = new pnode_t(a, 0);
#ifdef WITH_EVAL
        for (int i = 0; funcs[i].name(); i++) {
            if (lstring::cieq(vu.string, funcs[i].name())) {
                p->p_eval = funcs[i].func();
                p->p_string = vu.string;
                break;
            }
        }
#else
        delete [] vu.string;
#endif
        vu.string = 0;
        return (p);
    }


    pnode_t *
    elem_t::makeUnode(pnode_t *a, void*)
    {
        pnode_t *p = new pnode_t(a, 0);
#ifdef WITH_EVAL
        for (int i = 0; uops[i].name(); i++) {
            if (ops[i].optype() == token) {
                p->p_eval = uops[i].func();
                break;
            }
        }
#endif
        return (p);
    }


    pnode_t *
    elem_t::makeSnode(void*)
    {
        pnode_t *p = new pnode_t(0, 0);
#ifdef WITH_EVAL
        p->p_string = vu.string;
        p->p_eval = v_variable;
#else
        delete [] vu.string;
#endif
        vu.string = 0;
        return (p);
    }


    pnode_t *
    elem_t::makeNnode(void*)
    {
        pnode_t *p = new pnode_t(0, 0);
#ifdef WITH_EVAL
        p->p_val = vu.real;
        p->p_eval = v_constant;
#endif
        return (p);
    }


    char *
    elem_t::userString(const char**, bool)
    {
        return (0);
    }
    // End of elem_t functions.


    char *strip_keywords(const char*);
}


#ifdef WITH_EVAL

double *
cSced::evalExpr(const char **expr)
{
    static double d;

    Parser P(new elem_t[STACKSIZE], PRSR_UNITS | PRSR_AMPHACK | PRSR_NODEHACK);
    P.init(*expr, 0);
    pnode_t *p = P.parse();
    if (!p) {
#ifdef DEBUG
        const char *errmsg = P.getErrMesg();
        if (!errmsg)
            errmsg = "parse failed, unknown error";
        fprintf(stderr, "%s\n", errmsg);
#endif
        return (0);
    }
    if (!p->p_eval) {
        delete p;
        return (0);
    }
    *expr = P.residue();
    d = p->p_eval(p);
    delete p;
    return (&d);
}

#endif


// This function uses the expression parser ripped from WRspice to
// process a plot command line.  The return is a string consisting of
// double-quoted expressions, each quoted expression corresponds to a
// trace that would appear in the plot.
//
// The command string is processed as follows:
// 1.  We first remove any keyword assignments from the command.
// 2.  We identify separate expressions and add quoting.
// 3.  Finally, we check for a "vs" clause, and if found remove it
//     and the following expression.
//
// On error, a null string is returned.
//
char *
cSced::findPlotExpressions(const char *cstr)
{
    if (!cstr)
        return (0);
    char *string = strip_keywords(cstr);
    if (!string)
        return (lstring::copy(""));

    const char *thisone = string;
    Parser P(new elem_t[STACKSIZE], PRSR_UNITS | PRSR_AMPHACK | PRSR_NODEHACK);
    P.init(string, 0);

    bool found_vs = false;
    bool last_was_vs = false;
    sLstr lstr;
    while (*P.residue() != 0) {
        pnode_t *p = P.parse();
        if (!p) {
#ifdef DEBUG
            const char *errmsg = P.getErrMesg();
            if (!errmsg)
                errmsg = "parse failed, unknown error";
            fprintf(stderr, "%s\n", errmsg);
            delete [] string;
#endif
            return (0);
        }
        delete p;

        while (isspace(*thisone))
            thisone++;
        char *s;
        char *bf = new char[P.residue() - thisone + 1];
        for (s = bf; thisone < P.residue(); s++, thisone++)
            *s = *thisone;
        do {s--;}
        while (isspace(*s) && s != bf);
        *(s+1) = '\0';

        if (lstring::cieq(bf, "vs")) {
            if (found_vs) {
                // error
#ifdef DEBUG
                fprintf(stderr, "parse failed, only one \"vs\" allowed.\n");
#endif
                delete [] string;
                return (0);
            }
            last_was_vs = true;
            found_vs = true;
        }
        else if (!last_was_vs) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add_c('"');
            lstr.add(bf);
            lstr.add_c('"');
        }
        else
            last_was_vs = false;
        delete [] bf;
    }
    delete [] string;
    return (lstr.string_trim());
}


namespace {
    // A getqtok that returns the outermost quote char.
    char *get_qtok(char **str, char *qc)
    {
        while (isspace(**str))
            (*str)++;
        if (**str == '"')
            *qc = '"';
        else if (**str == '\'')
            *qc = '\'';
        else
            *qc = 0;
        return (lstring::getqtok(str));
    }

    // the scaletype arguments
    const char *kw_multi            = "multi";
    const char *kw_single           = "single";
    const char *kw_group            = "group";

    // the plotstyle arguments
    const char *kw_linplot          = "linplot";
    const char *kw_pointplot        = "pointplot";
    const char *kw_combplot         = "combplot";

    // the gridstyle arguments
    const char *kw_lingrid          = "lingrid";
    const char *kw_xlog             = "xlog";
    const char *kw_ylog             = "ylog";
    const char *kw_loglog           = "loglog";
    const char *kw_polar            = "polar";
    const char *kw_smith            = "smith";
    const char *kw_smithgrid        = "smithgrid";

    // the plot keywords
    //const char *kw_curplot          = "curplot";
    //const char *kw_device           = "device";
    //const char *kw_gridsize         = "gridsize";
    //const char *kw_gridstyle        = "gridstyle";
    const char *kw_nogrid           = "nogrid";
    const char *kw_nointerp         = "nointerp";
    //const char *kw_plotstyle        = "plotstyle";
    //const char *kw_pointchars       = "pointchars";
    //const char *kw_polydegree       = "polydegree";
    //const char *kw_polysteps        = "polysteps";
    //const char *kw_scaletype        = "scaletype";
    //const char *kw_ticmarks         = "ticmarks";
    const char *kw_title            = "title";
    const char *kw_xcompress        = "xcompress";
    const char *kw_xdelta           = "xdelta";
    const char *kw_xindices         = "xindices";
    const char *kw_xlabel           = "xlabel";
    const char *kw_xlimit           = "xlimit";
    const char *kw_ydelta           = "ydelta";
    const char *kw_ylabel           = "ylabel";
    const char *kw_ylimit           = "ylimit";
    const char *kw_ysep             = "ysep";


    // Remove the keyword assignments from the plot expression list,
    // returning a list containing expressions only.
    //
    char *
    strip_keywords(const char *cstr)
    {
        sLstr ls_opt;
        sLstr ls_plot;
        char *string = lstring::copy(cstr);
        char *s = string;

        // This is a bit tricky.  We want to allow an optional '='
        // between keywords and values.  We also support N N and N,N
        // for the keywords that take two numbers.

        char *tok;
        char *lastpos = s;
        while ((tok = lstring::getqtok(&s, "=")) != 0) {
            bool handled = false;
            if (lstring::cieq(tok, kw_xlimit) ||
                    lstring::cieq(tok, kw_ylimit) ||
                    lstring::cieq(tok, kw_xindices)) {

                char *tok1, *tok2;
                if (*s == '"') {
                    tok1 = lstring::gettok(&s, ",\"");
                    tok2 = lstring::gettok(&s, "\"");
                }
                else {
                    tok1 = lstring::gettok(&s, ",");
                    tok2 = lstring::gettok(&s);
                }
                if (tok1 && tok2) {
                    if (ls_opt.length())
                        ls_opt.add_c(' ');
                    ls_opt.add(tok);
                    ls_opt.add_c(' ');
                    ls_opt.add(tok1);
                    ls_opt.add_c(' ');
                    ls_opt.add(tok2);
                }
                delete [] tok1;
                delete [] tok2;
                handled = true;
            }
            else if (lstring::cieq(tok, kw_xcompress) ||
                    lstring::cieq(tok, kw_xdelta) ||
                    lstring::cieq(tok, kw_ydelta)) {

                char *tok1 = lstring::gettok(&s);
                if (tok1) {
                    if (ls_opt.length())
                        ls_opt.add_c(' ');
                    ls_opt.add(tok);
                    ls_opt.add_c(' ');
                    ls_opt.add(tok1);
                    ls_opt.add_c(' ');
                }
                delete [] tok1;
                handled = true;
            }
            else if (lstring::cieq(tok, kw_nointerp) ||
                    lstring::cieq(tok, kw_ysep) ||
                    lstring::cieq(tok, kw_nogrid) ||

                    lstring::cieq(tok, kw_lingrid) ||
                    lstring::cieq(tok, kw_xlog) ||
                    lstring::cieq(tok, kw_ylog) ||
                    lstring::cieq(tok, kw_loglog) ||
                    lstring::cieq(tok, kw_polar) ||
                    lstring::cieq(tok, kw_smith) ||
                    lstring::cieq(tok, kw_smithgrid) ||

                    lstring::cieq(tok, kw_linplot) ||
                    lstring::cieq(tok, kw_pointplot) ||
                    lstring::cieq(tok, kw_combplot) ||

                    lstring::cieq(tok, kw_multi) ||
                    lstring::cieq(tok, kw_single) ||
                    lstring::cieq(tok, kw_group)) {

                if (ls_opt.length())
                    ls_opt.add_c(' ');
                ls_opt.add(tok);
                handled = true;
            }
            else if (lstring::cieq(tok, kw_xlabel) ||
                    lstring::cieq(tok, kw_ylabel) ||
                    lstring::cieq(tok, kw_title)) {

                char *tok1 = lstring::getqtok(&s);
                if (tok1) {
                    if (ls_opt.length())
                        ls_opt.add_c(' ');
                    ls_opt.add(tok);
                    ls_opt.add_c(' ');
                    ls_opt.add_c('"');
                    ls_opt.add(tok1);
                    ls_opt.add_c('"');
                    ls_opt.add_c(' ');
                }
                delete [] tok1;
                handled = true;
            }
            delete [] tok;
            if (handled) {
                lastpos = s;
                continue;
            }

            // Not a keyword, back up and parse as an expression fragment.
            //
            s = lastpos;
            char qc;
            tok = get_qtok(&s, &qc);

            if (ls_plot.length())
                ls_plot.add_c(' ');

            // preserve quoting, if any
            if (qc)
                ls_plot.add_c(qc);
            ls_plot.add(tok);
            if (qc)
                ls_plot.add_c(qc);
            lastpos = s;
            delete [] tok;
        }
        delete [] string;

        return (ls_plot.string_trim());
    }
}

//-----------------------------------------------------------------------------

#ifdef WITH_EVAL


namespace {
    // Return the first function argument.
    double arg1(pnode_t *p)
    {
        if (p) {
            if (p->p_eval == &v_comma)
                p = p->p_left;
            if (p && p->p_eval)
                return (p->p_eval(p));
        }
        return (0);
    }

    // Return the second function argument.  The supported functions
    // take at most two arguments.
    double arg2(pnode_t *p) {
        if (p && p->p_eval == &v_comma) {
            p = p->p_right;
            if (p->p_eval == &v_comma)
                p = p->p_left;
            if (p && p->p_eval)
                return (p->p_eval(p));
        }
        return (0.0);
    }

    double v_constant(pnode_t *p) { return (p ? p->p_val : 0.0); }
    double v_variable(pnode_t *) { return (0.0); }

    double v_plus(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a + b);
            }
            return (0.0);
        }

    double v_minus(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a - b);
            }
            return (0.0);
        }

    double v_times(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a * b);
            }
            return (0.0);
        }

    double v_mod(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                if (b == 0.0)
                    return (0.0);
                return (fmod(a, b));
            }
            return (0.0);
        }

    double v_divide(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                if (b == 0.0)
                    return (0.0);
                return (a / b);
            }
            return (0.0);
        }

    double v_comma(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                if (pl && pl->p_eval)
                    pl->p_eval(pl);
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (b);
            }
            return (0.0);
        }

    double v_power(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (pow(a, b));
            }
            return (0.0);
        }

    double v_eq(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a == b);
            }
            return (0.0);
        }

    double v_gt(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a > b);
            }
            return (0.0);
        }

    double v_lt(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a < b);
            }
            return (0.0);
        }

    double v_ge(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a >= b);
            }
            return (0.0);
        }

    double v_le(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a <= b);
            }
            return (0.0);
        }

    double v_ne(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return (a != b);
            }
            return (0.0);
        }

    double v_and(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return ((int)a && (int)b);
            }
            return (0.0);
        }

    double v_or(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                return ((int)a || (int)b);
            }
            return (0.0);
        }

    double v_cond(pnode_t *p)
        {
            // a ? b : c
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                pnode_t *pr = p->p_right;
                if (pr) {
                    if (mmRnd(a) != 0)
                        pr = pr->p_left;
                    else
                        pr = pr->p_right;
                    double b = pr && pr->p_eval ? pr->p_eval(pr) : 0.0;
                    return (b);
                }
            }
            return (0.0);
        }

    double v_uminus(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                return (-a);
            }
            return (0.0);
        }

    double v_not(pnode_t *p)
        {
            if (p) {
                pnode_t *pl = p->p_left;
                double a = pl && pl->p_eval ? pl->p_eval(pl) : 0.0;
                return (!(int)a);
            }
            return (0.0);
        }


    double v_mag(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (fabs(a));
            }
            return (0.0);
        }

    double v_ph(pnode_t *) { return (0.0); }
    double v_j(pnode_t *) { return (0.0); }

    double v_real(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (a);
            }
            return (0.0);
        }

    double v_imag(pnode_t *) { return (0.0); }

    double v_db(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                if (a <= 0.0)
                    return (0.0);
                return (20*log10(a));
            }
            return (0.0);
        }

    double v_log10(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                if (a <= 0.0)
                    return (0.0);
                return (log10(a));
            }
            return (0.0);
        }

    double v_log(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                if (a <= 0.0)
                    return (0.0);
                return (log(a));
            }
            return (0.0);
        }

    double v_ln(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                if (a <= 0.0)
                    return (0.0);
                return (log(a));
            }
            return (0.0);
        }

    double v_exp(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (exp(a));
            }
            return (0.0);
        }

    double v_sqrt(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                if (a < 0.0)
                    return (0.0);
                return (sqrt(a));
            }
            return (0.0);
        }

    double v_sin(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (sin(a));
            }
            return (0.0);
        }

    double v_cos(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (cos(a));
            }
            return (0.0);
        }

    double v_tan(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (tan(a));
            }
            return (0.0);
        }

    double v_asin(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (asin(a));
            }
            return (0.0);
        }

    double v_acos(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (acos(a));
            }
            return (0.0);
        }

    double v_atan(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (atan(a));
            }
            return (0.0);
        }

    double v_sinh(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (sinh(a));
            }
            return (0.0);
        }

    double v_cosh(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (cosh(a));
            }
            return (0.0);
        }

    double v_tanh(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (tanh(a));
            }
            return (0.0);
        }

    double v_asinh(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (asinh(a));
            }
            return (0.0);
        }

    double v_acosh(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (acosh(a));
            }
            return (0.0);
        }

    double v_atanh(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (atanh(a));
            }
            return (0.0);
        }

    double v_j0(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (j0(a));
            }
            return (0.0);
        }

    double v_j1(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (j1(a));
            }
            return (0.0);
        }

    double v_jn(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                double b = arg2(p->p_left);
                int n = mmRnd(a);
                return (jn(n, b));
            }
            return (0.0);
        }

    double v_y0(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (y0(a));
            }
            return (0.0);
        }

    double v_y1(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (y1(a));
            }
            return (0.0);
        }

    double v_yn(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                double b = arg2(p->p_left);
                int n = mmRnd(a);
                return (yn(n, b));
            }
            return (0.0);
        }

    double v_cbrt(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (cbrt(a));
            }
            return (0.0);
        }

    double v_erf(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (erf(a));
            }
            return (0.0);
        }

    double v_erfc(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (erfc(a));
            }
            return (0.0);
        }

    double v_gamma(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (tgamma(a));
            }
            return (0.0);
        }

    double v_norm(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (fabs(a));
            }
            return (0.0);
        }

    double v_rnd(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (mmRnd(a));
            }
            return (0.0);
        }

    double v_ogauss(pnode_t *) { return (0.0); }
    double v_fft(pnode_t *) { return (0.0); }
    double v_ifft(pnode_t *) { return (0.0); }
    double v_pos(pnode_t *) { return (0.0); }
    double v_mean(pnode_t *) { return (0.0); }
    double v_vector(pnode_t *) { return (0.0); }
    double v_unitvec(pnode_t *) { return (0.0); }
    double v_size(pnode_t *) { return (0.0); }
    double v_interpolate(pnode_t *) { return (0.0); }
    double v_deriv(pnode_t *) { return (0.0); }
    double v_integ(pnode_t *) { return (0.0); }
    double v_rms(pnode_t *) { return (0.0); }
    double v_sum(pnode_t *) { return (0.0); }

    double v_floor(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (floor(a));
            }
            return (0.0);
        }

    double v_sgn(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                if (a > 0.0)
                    return (1.0);
                if (a < 0.0)
                    return (-1.0);
            }
            return (0.0);
        }

    double v_ceil(pnode_t *p)
        {
            if (p) {
                double a = arg1(p->p_left);
                return (ceil(a));
            }
            return (0.0);
        }

    double v_rint(pnode_t *) { return (0.0); }
    double v_hs_unif(pnode_t *) { return (0.0); }
    double v_hs_aunif(pnode_t *) { return (0.0); }
    double v_hs_gauss(pnode_t *) { return (0.0); }
    double v_hs_agauss(pnode_t *) { return (0.0); }
    double v_hs_limit(pnode_t *) { return (0.0); }
    double v_hs_pow(pnode_t *) { return (0.0); }
    double v_hs_pwr(pnode_t *) { return (0.0); }
    double v_hs_sign(pnode_t *) { return (0.0); }
}

#endif

