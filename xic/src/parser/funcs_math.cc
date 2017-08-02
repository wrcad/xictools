
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

#include "config.h"
#include "cd.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_lexpr.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_scrfunc.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "random.h"
#include "tvals.h"
#include <time.h>
#include <errno.h>
#ifdef HAVE_FENV_H
#include <fenv.h>
#endif


//
// Core functions and operators used in the parse tree.
//

//#define TIMEDBG

#ifndef M_PI
#define	M_PI        3.14159265358979323846  // pi
#define M_PI_2      1.57079632679489661923  // pi/2
#define M_LOG10E    0.43429448190325182765  // log_10 e
#endif

// Hack:  The log() function was changed to natural log in 3.2.23 for
// compatibility with WRspice.  Setting this will revert to log10.
//
bool SIinterp::siLogIsLog10;

namespace {
    namespace math_funcs {
        // Operator functions
        bool PTplus(Variable*, Variable*, void*);
        bool PTminus(Variable*, Variable*, void*);
        bool PTtimes(Variable*, Variable*, void*);
        bool PTmod(Variable*, Variable*, void*);
        bool PTdivide(Variable*, Variable*, void*);
        bool PTpower(Variable*, Variable*, void*);
        bool PTeq(Variable*, Variable*, void*);
        bool PTgt(Variable*, Variable*, void*);
        bool PTlt(Variable*, Variable*, void*);
        bool PTge(Variable*, Variable*, void*);
        bool PTle(Variable*, Variable*, void*);
        bool PTne(Variable*, Variable*, void*);
        bool PTand(Variable*, Variable*, void*);
        bool PTor(Variable*, Variable*, void*);
        bool PTuminus(Variable*, Variable*, void*);
        bool PTnot(Variable*, Variable*, void*);

        // Complex numbers
        bool PTcmplx(Variable*, Variable*, void*);
        bool PTreal(Variable*, Variable*, void*);
        bool PTimag(Variable*, Variable*, void*);
        bool PTmag(Variable*, Variable*, void*);
        bool PTang(Variable*, Variable*, void*);

        // Math functions
        bool PTabs(Variable*, Variable*, void*);
        bool PTsgn(Variable*, Variable*, void*);
        bool PTacos(Variable*, Variable*, void*);
        bool PTasin(Variable*, Variable*, void*);
        bool PTatan(Variable*, Variable*, void*);
#ifdef HAVE_ACOSH
        bool PTacosh(Variable*, Variable*, void*);
        bool PTasinh(Variable*, Variable*, void*);
        bool PTatanh(Variable*, Variable*, void*);
#endif // HAVE_ACOSH
        bool PTcbrt(Variable*, Variable*, void*);
        bool PTcos(Variable*, Variable*, void*);
        bool PTcosh(Variable*, Variable*, void*);
        bool PTerf(Variable*, Variable*, void*);
        bool PTerfc(Variable*, Variable*, void*);
        bool PTexp(Variable*, Variable*, void*);
        bool PTj0(Variable*, Variable*, void*);
        bool PTj1(Variable*, Variable*, void*);
        bool PTjn(Variable*, Variable*, void*);
        bool PTy0(Variable*, Variable*, void*);
        bool PTy1(Variable*, Variable*, void*);
        bool PTyn(Variable*, Variable*, void*);
        bool PTln(Variable*, Variable*, void*);
        bool PTlog(Variable*, Variable*, void*);
        bool PTlog10(Variable*, Variable*, void*);
        bool PTsin(Variable*, Variable*, void*);
        bool PTsinh(Variable*, Variable*, void*);
        bool PTsqrt(Variable*, Variable*, void*);
        bool PTtan(Variable*, Variable*, void*);
        bool PTtanh(Variable*, Variable*, void*);
        bool PTatan2(Variable*, Variable*, void*);
        bool PTseed(Variable*, Variable*, void*);
        bool PTrandom(Variable*, Variable*, void*);
        bool PTgauss(Variable*, Variable*, void*);
        bool PTfloor(Variable*, Variable*, void*);
        bool PTceil(Variable*, Variable*, void*);
        bool PTrint(Variable*, Variable*, void*);
        bool PTint(Variable*, Variable*, void*);
        bool PTmax(Variable*, Variable*, void*);
        bool PTmin(Variable*, Variable*, void*);
    }
    using namespace math_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Complex numbers
    PY_FUNC(cmplx,  2,  PTcmplx);
    PY_FUNC(real,   1,  PTreal);
    PY_FUNC(imag,   1,  PTimag);
    PY_FUNC(mag,    1,  PTmag);
    PY_FUNC(ang,    1,  PTang);

    // Math functions
    PY_FUNC(abs,    1,  PTabs);
    PY_FUNC(acos,   1,  PTacos);
    PY_FUNC(asin,   1,  PTasin);
    PY_FUNC(atan,   1,  PTatan);
#ifdef HAVE_ACOSH
    PY_FUNC(acosh,  1,  PTacosh);
    PY_FUNC(asinh,  1,  PTasinh);
    PY_FUNC(atanh,  1,  PTatanh);
#endif
    PY_FUNC(cbrt,   1,  PTcbrt);
    PY_FUNC(cos,    1,  PTcos);
    PY_FUNC(cosh,   1,  PTcosh);
    PY_FUNC(j0,     1,  PTj0);
    PY_FUNC(j1,     1,  PTj1);
    PY_FUNC(jn,     2,  PTjn);
    PY_FUNC(y0,     1,  PTy0);
    PY_FUNC(y1,     1,  PTy1);
    PY_FUNC(yn,     2,  PTyn);
    PY_FUNC(erf,    1,  PTerf);
    PY_FUNC(erfc,   1,  PTerfc);
    PY_FUNC(exp,    1,  PTexp);
    PY_FUNC(ln,     1,  PTln);
    PY_FUNC(log,    1,  PTlog);
    PY_FUNC(log10,  1,  PTlog10);
    PY_FUNC(pow,    2,  PTpower);
    PY_FUNC(sgn,    1,  PTsgn);
    PY_FUNC(sin,    1,  PTsin);
    PY_FUNC(sinh,   1,  PTsinh);
    PY_FUNC(sqrt,   1,  PTsqrt);
    PY_FUNC(tan,    1,  PTtan);
    PY_FUNC(tanh,   1,  PTtanh);
    PY_FUNC(atan2,  2,  PTatan2);
    PY_FUNC(seed,   1,  PTseed);
    PY_FUNC(random, 0,  PTrandom);
    PY_FUNC(gauss,  0,  PTgauss);
    PY_FUNC(floor,  1,  PTfloor);
    PY_FUNC(ceil,   1,  PTceil);
    PY_FUNC(rint,   1,  PTrint);
    PY_FUNC(int,    1,  PTint);
    PY_FUNC(max,    2,  PTmax);
    PY_FUNC(min,    2,  PTmin);

    void py_register_math()
    {
      // Complex numbers
      cPyIf::register_func("cmplx",  pycmplx);
      cPyIf::register_func("real",   pyreal);
      cPyIf::register_func("imag",   pyimag);
      cPyIf::register_func("mag",    pymag);
      cPyIf::register_func("ang",    pyang);

      // Math functions
      cPyIf::register_func("abs",    pyabs);
      cPyIf::register_func("acos",   pyacos);
      cPyIf::register_func("asin",   pyasin);
      cPyIf::register_func("atan",   pyatan);
#ifdef HAVE_ACOSH
      cPyIf::register_func("acosh",  pyacosh);
      cPyIf::register_func("asinh",  pyasinh);
      cPyIf::register_func("atanh",  pyatanh);
#endif
      cPyIf::register_func("cbrt",   pycbrt);
      cPyIf::register_func("cos",    pycos);
      cPyIf::register_func("cosh",   pycosh);
      cPyIf::register_func("j0",     pyj0);
      cPyIf::register_func("j1",     pyj1);
      cPyIf::register_func("jn",     pyjn);
      cPyIf::register_func("y0",     pyy0);
      cPyIf::register_func("y1",     pyy1);
      cPyIf::register_func("yn",     pyyn);
      cPyIf::register_func("erf",    pyerf);
      cPyIf::register_func("erfc",   pyerfc);
      cPyIf::register_func("exp",    pyexp);
      cPyIf::register_func("ln",     pyln);
      cPyIf::register_func("log",    pylog);
      cPyIf::register_func("pow",    pypow);
      cPyIf::register_func("log10",  pylog10);
      cPyIf::register_func("sgn",    pysgn);
      cPyIf::register_func("sin",    pysin);
      cPyIf::register_func("sinh",   pysinh);
      cPyIf::register_func("sqrt",   pysqrt);
      cPyIf::register_func("tan",    pytan);
      cPyIf::register_func("tanh",   pytanh);
      cPyIf::register_func("atan2",  pyatan2);
      cPyIf::register_func("seed",   pyseed);
      cPyIf::register_func("random", pyrandom);
      cPyIf::register_func("gauss",  pygauss);
      cPyIf::register_func("floor",  pyfloor);
      cPyIf::register_func("ceil",   pyceil);
      cPyIf::register_func("rint",   pyrint);
      cPyIf::register_func("int",    pyint);
      cPyIf::register_func("max",    pymax);
      cPyIf::register_func("min",    pymin);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // Tcl/Tk wrappers.

    // Complex numbers
    TCL_FUNC(cmplx,  2,  PTcmplx);
    TCL_FUNC(real,   1,  PTreal);
    TCL_FUNC(imag,   1,  PTimag);
    TCL_FUNC(mag,    1,  PTmag);
    TCL_FUNC(ang,    1,  PTang);

    // Math functions
    TCL_FUNC(abs,    1,  PTabs);
    TCL_FUNC(acos,   1,  PTacos);
    TCL_FUNC(asin,   1,  PTasin);
    TCL_FUNC(atan,   1,  PTatan);
#ifdef HAVE_ACOSH
    TCL_FUNC(acosh,  1,  PTacosh);
    TCL_FUNC(asinh,  1,  PTasinh);
    TCL_FUNC(atanh,  1,  PTatanh);
#endif
    TCL_FUNC(cbrt,   1,  PTcbrt);
    TCL_FUNC(cos,    1,  PTcos);
    TCL_FUNC(cosh,   1,  PTcosh);
    TCL_FUNC(j0,     1,  PTj0);
    TCL_FUNC(j1,     1,  PTj1);
    TCL_FUNC(jn,     2,  PTjn);
    TCL_FUNC(y0,     1,  PTy0);
    TCL_FUNC(y1,     1,  PTy1);
    TCL_FUNC(yn,     2,  PTyn);
    TCL_FUNC(erf,    1,  PTerf);
    TCL_FUNC(erfc,   1,  PTerfc);
    TCL_FUNC(exp,    1,  PTexp);
    TCL_FUNC(ln,     1,  PTln);
    TCL_FUNC(log,    1,  PTlog);
    TCL_FUNC(log10,  1,  PTlog10);
    TCL_FUNC(pow,    2,  PTpower);
    TCL_FUNC(sgn,    1,  PTsgn);
    TCL_FUNC(sin,    1,  PTsin);
    TCL_FUNC(sinh,   1,  PTsinh);
    TCL_FUNC(sqrt,   1,  PTsqrt);
    TCL_FUNC(tan,    1,  PTtan);
    TCL_FUNC(tanh,   1,  PTtanh);
    TCL_FUNC(atan2,  2,  PTatan2);
    TCL_FUNC(seed,   1,  PTseed);
    TCL_FUNC(random, 0,  PTrandom);
    TCL_FUNC(gauss,  0,  PTgauss);
    TCL_FUNC(floor,  1,  PTfloor);
    TCL_FUNC(ceil,   1,  PTceil);
    TCL_FUNC(rint,   1,  PTrint);
    TCL_FUNC(int,    1,  PTint);
    TCL_FUNC(max,    2,  PTmax);
    TCL_FUNC(min,    2,  PTmin);

    void tcl_register_math()
    {
      // Complex numbers
      cTclIf::register_func("cmplx",  tclcmplx);
      cTclIf::register_func("real",   tclreal);
      cTclIf::register_func("imag",   tclimag);
      cTclIf::register_func("mag",    tclmag);
      cTclIf::register_func("ang",    tclang);

      // Math functions
      cTclIf::register_func("abs",    tclabs);
      cTclIf::register_func("acos",   tclacos);
      cTclIf::register_func("asin",   tclasin);
      cTclIf::register_func("atan",   tclatan);
#ifdef HAVE_ACOSH
      cTclIf::register_func("acosh",  tclacosh);
      cTclIf::register_func("asinh",  tclasinh);
      cTclIf::register_func("atanh",  tclatanh);
#endif
      cTclIf::register_func("cbrt",   tclcbrt);
      cTclIf::register_func("cos",    tclcos);
      cTclIf::register_func("cosh",   tclcosh);
      cTclIf::register_func("j0",     tclj0);
      cTclIf::register_func("j1",     tclj1);
      cTclIf::register_func("jn",     tcljn);
      cTclIf::register_func("y0",     tcly0);
      cTclIf::register_func("y1",     tcly1);
      cTclIf::register_func("yn",     tclyn);
      cTclIf::register_func("erf",    tclerf);
      cTclIf::register_func("erfc",   tclerfc);
      cTclIf::register_func("exp",    tclexp);
      cTclIf::register_func("ln",     tclln);
      cTclIf::register_func("log",    tcllog);
      cTclIf::register_func("log10",  tcllog10);
      cTclIf::register_func("pow",    tclpow);
      cTclIf::register_func("sgn",    tclsgn);
      cTclIf::register_func("sin",    tclsin);
      cTclIf::register_func("sinh",   tclsinh);
      cTclIf::register_func("sqrt",   tclsqrt);
      cTclIf::register_func("tan",    tcltan);
      cTclIf::register_func("tanh",   tcltanh);
      cTclIf::register_func("atan2",  tclatan2);
      cTclIf::register_func("seed",   tclseed);
      cTclIf::register_func("random", tclrandom);
      cTclIf::register_func("gauss",  tclgauss);
      cTclIf::register_func("floor",  tclfloor);
      cTclIf::register_func("ceil",   tclceil);
      cTclIf::register_func("rint",   tclrint);
      cTclIf::register_func("int",    tclint);
      cTclIf::register_func("max",    tclmax);
      cTclIf::register_func("min",    tclmin);
    }
#endif  // HAVE_TCL
}

#define MODULUS(NUM,LIMIT)  ((NUM) - ((int) ((NUM) / (LIMIT))) * (LIMIT))

SIptop SIparser::spPTops[] = {

    {0,      0 },                      // TOK_END
    {"+",    math_funcs::PTplus },     // TOK_PLUS
    {"-",    math_funcs::PTminus },    // TOK_MINUS
    {"*",    math_funcs::PTtimes },    // TOK_TIMES
    {"%",    math_funcs::PTmod },      // TOK_MOD
    {"/",    math_funcs::PTdivide },   // TOK_DIVIDE
    {"^",    math_funcs::PTpower },    // TOK_POWER
    {"==",   math_funcs::PTeq },       // TOK_EQ
    {">",    math_funcs::PTgt },       // TOK_GT
    {"<",    math_funcs::PTlt },       // TOK_LT
    {">=",   math_funcs::PTge },       // TOK_GE
    {"<=",   math_funcs::PTle },       // TOK_LE
    {"!=",   math_funcs::PTne },       // TOK_NE
    {"&",    math_funcs::PTand },      // TOK_AND
    {"|",    math_funcs::PTor },       // TOK_OR
    {",",    0 },                      // TOK_COMMA
    {"?",    0 },                      // TOK_COND
    {":",    0 },                      // TOK_COLON
    {"=",    0 },                      // TOK_ASSIGN
    {"-",    math_funcs::PTuminus },   // TOK_UMINUS
    {"!",    math_funcs::PTnot },      // TOK_NOT
    {"++",   0 },                      // TOK_LINCR
    {"--",   0 },                      // TOK_LDECR
    {"++",   0 },                      // TOK_RINCR
    {"--",   0 },                      // TOK_RDECR
};


// Export to load functions in this script library.
//
void
SIparser::funcs_math_init()
{
  using namespace math_funcs;

  // Complex numbers
  registerFunc("cmplx",  2,  PTcmplx);
  registerFunc("real",   1,  PTreal);
  registerFunc("imag",   1,  PTimag);
  registerFunc("mag",    1,  PTmag);
  registerFunc("ang",    1,  PTang);

  // Math functions
  registerFunc("abs",    1,  PTabs);
  registerFunc("acos",   1,  PTacos);
  registerFunc("asin",   1,  PTasin);
  registerFunc("atan",   1,  PTatan);
#ifdef HAVE_ACOSH
  registerFunc("acosh",  1,  PTacosh);
  registerFunc("asinh",  1,  PTasinh);
  registerFunc("atanh",  1,  PTatanh);
#endif
  registerFunc("cbrt",   1,  PTcbrt);
  registerFunc("cos",    1,  PTcos);
  registerFunc("cosh",   1,  PTcosh);
  registerFunc("j0",     1,  PTj0);
  registerFunc("j1",     1,  PTj1);
  registerFunc("jn",     1,  PTjn);
  registerFunc("y0",     1,  PTy0);
  registerFunc("y1",     1,  PTy1);
  registerFunc("yn",     1,  PTyn);
  registerFunc("erf",    1,  PTerf);
  registerFunc("erfc",   1,  PTerfc);
  registerFunc("exp",    1,  PTexp);
  registerFunc("ln",     1,  PTln);
  registerFunc("log",    1,  PTlog);
  registerFunc("log10",  1,  PTlog10);
  registerFunc("pow",    2,  PTpower);
  registerFunc("sgn",    1,  PTsgn);
  registerFunc("sin",    1,  PTsin);
  registerFunc("sinh",   1,  PTsinh);
  registerFunc("sqrt",   1,  PTsqrt);
  registerFunc("tan",    1,  PTtan);
  registerFunc("tanh",   1,  PTtanh);
  registerFunc("atan2",  2,  PTatan2);
  registerFunc("seed",   1,  PTseed);
  registerFunc("random", 0,  PTrandom);
  registerFunc("gauss",  0,  PTgauss);
  registerFunc("floor",  1,  PTfloor);
  registerFunc("ceil",   1,  PTceil);
  registerFunc("rint",   1,  PTrint);
  registerFunc("int",    1,  PTint);
  registerFunc("max",    2,  PTmax);
  registerFunc("min",    2,  PTmin);

#ifdef HAVE_PYTHON
  py_register_math();
#endif
#ifdef HAVE_TCL
  tcl_register_math();
#endif
}

// Value returned when out of range.  This allows a little slop for
// processing, unlike MAXDOUBLE
//
#define HUGENUM  1.0e+300
#define TINYNUM  1.0e-300

inline bool is_scalar(int n, Variable *args)
{
    return (args[n].type == TYP_SCALAR);
}

inline bool is_cmplx(int n, Variable *args)
{
    return (args[n].type == TYP_CMPLX);
}

inline bool is_string(int n, Variable *args)
{
    return (args[n].type == TYP_STRING || args[n].type == TYP_NOTYPE ||
        (args[n].type == TYP_SCALAR && args[n].content.string == 0));
}

inline bool is_zlist(int n, Variable *args)
{
    return (args[n].type == TYP_ZLIST);
}

inline bool is_array(int n, Variable *args)
{
    return (args[n].type == TYP_ARRAY);
}

inline bool is_handle(int n, Variable *args)
{
    return (args[n].type == TYP_HANDLE);
}

inline Cmplx *cmplx(Variable *res)
{
    res->type = TYP_CMPLX;
    return (&res->content.cx);
}

inline double real(const Variable &v)
{
    return (v.content.cx.real);
}

inline double imag(const Variable &v)
{
    return (v.content.cx.imag);
}

//
// Error handling.  In Linux (CentOS 7), most functions use fenv,
// some such as the Bessels use errno==ERANGE. 
//

inline void init_fpe()
{
#ifdef HAVE_FENV_H
    feclearexcept(FE_ALL_EXCEPT);
#endif
}

inline bool check_fpe(const char *name)
{
#ifdef HAVE_FENV_H
    unsigned int f =
        fetestexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW | FE_INVALID);
    if (!f)
        return (false);
    if (f & FE_DIVBYZERO) {
        feclearexcept(FE_ALL_EXCEPT);
        Errs()->add_error("%s: floating-point error, divide by zero.\n", name);
        return (true);
    }
    if (f & FE_OVERFLOW) {
        feclearexcept(FE_ALL_EXCEPT);
        Errs()->add_error("%s: floating-point error, overflow.\n", name);
        return (true);
    }
    if (f & FE_UNDERFLOW) {
        // Don't really care about this one.
        feclearexcept(FE_ALL_EXCEPT);
        return (false);
    }
    if (f & FE_INVALID) {
        feclearexcept(FE_ALL_EXCEPT);
        Errs()->add_error("%s: floating-point error, invalid result.\n", name);
        return (true);
    }
#else
    (void)name;
#endif
    return (false);
}

inline bool check_rng(const char *name)
{
    if (errno == ERANGE) {
        Errs()->add_error("%s: range error.", name);
        return (true);
    }
    if (errno == EDOM) {
        Errs()->add_error("%s: domain error.", name);
        return (true);
    }
    return (false);
}

//------------------------------------------------------------------------
// Operator functions
//------------------------------------------------------------------------

bool
math_funcs::PTplus(Variable *res, Variable *args, void *datap)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value = args[0].content.value + args[1].content.value;
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = args[0].content.value + real(args[1]);
            cx->imag = imag(args[1]);
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        if (is_scalar(1, args)) {
            cx->real = real(args[0]) + args[1].content.value;
            cx->imag = imag(args[0]);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            cx->real = real(args[0]) + real(args[1]);
            cx->imag = imag(args[0]) + imag(args[1]);
            return (OK);
        }
    }

    if (is_array(0, args) || is_array(1, args)) {
        if (is_scalar(0, args)) {
            int indx = (int)args[0].content.value;
            return (ADATA(args[1].content.a)->mkpointer(res, indx));
        }
        if (is_scalar(1, args)) {
            int indx = (int)args[1].content.value;
            return (ADATA(args[0].content.a)->mkpointer(res, indx));
        }
        return (BAD);
    }

    if (is_string(0, args)) {
        char *str1 = args[0].content.string;
        if (is_string(1, args)) {
            char *str2 = args[1].content.string;
            int l1 = str1 ? strlen(str1) : 0;
            int l2 = str2 ? strlen(str2) : 0;
            char *string = new char[l1 + l2 + 1];
            string[0] = 0;
            if (l1)
                strcpy(string, str1);
            if (l2)
                strcpy(string + l1, str2);
            res->type = TYP_STRING;
            res->content.string = string;
            res->flags |= VF_ORIGINAL;
            return (OK);
        }
        if (is_scalar(1, args)) {
            if (!str1)
                return (BAD);
            int indx = (int)args[1].content.value;
            if (indx < 0)
                // Don't allow negaitve indexing.
                return (BAD);
            if (indx >= (int)strlen(str1))
                return (BAD);
            res->type = TYP_STRING;
            res->content.string = str1 + indx;
            return (OK);
        }
        return (BAD);
    }
    if (is_string(1, args)) {
        char *str2 = args[1].content.string;
        if (!str2)
            return (BAD);
        if (is_scalar(0, args)) {
            int indx = (int)args[0].content.value;
            if (indx < 0)
                // Don't allow negaitve indexing.
            if (indx >= (int)strlen(str2))
                return (BAD);
            res->type = TYP_STRING;
            res->content.string = str2 + indx;
            return (OK);
        }
        return (BAD);
    }

    if (is_zlist(0, args) || is_zlist(1, args))
        return (zlist_funcs::PTorZ(res, args, datap));

    if (is_handle(0, args)) {
        if (is_handle(1, args)) {
            args[0].cat__handle(&args[1], res);
            return (OK);
        }
        if (is_scalar(1, args)) {
            int indx = (int)args[1].content.value;
            if (indx == 0) {
                res->type = TYP_SCALAR;
                res->content.value = 0.0;
                return (OK);
            }
        }
        return (BAD);
    }
    if (is_handle(1, args)) {
        if (is_scalar(0, args)) {
            int indx = 0;
            arg_int(args, 0, &indx);
            if (indx == 0) {
                res->type = TYP_SCALAR;
                res->content.value = 0.0;
                return (OK);
            }
        }
        return (BAD);
    }

    return (BAD);
}


bool
math_funcs::PTminus(Variable *res, Variable *args, void *datap)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value = args[0].content.value - args[1].content.value;
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = args[0].content.value - real(args[1]);
            cx->imag = -imag(args[1]);
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        if (is_scalar(1, args)) {
            cx->real = real(args[0]) - args[1].content.value;
            cx->imag = imag(args[0]);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            cx->real = real(args[0]) - real(args[1]);
            cx->imag = imag(args[0]) - imag(args[1]);
            return (OK);
        }
    }

    if (is_array(0, args)) {
        if (is_array(1, args)) {
            res->content.value =
                args[0].content.a->values() - args[1].content.a->values();
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_scalar(1, args)) {
            int indx = -(int)args[1].content.value;
            return (ADATA(args[0].content.a)->mkpointer(res, indx));
        }
        return (BAD);
    }

    if (is_string(0, args)) {
        char *str1 = args[0].content.string;
        if (!str1)
            return (BAD);
        if (is_string(1, args)) {
            char *str2 = args[1].content.string;
            if (!str2)
                return (BAD);
            res->content.value = str1 - str2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_scalar(1, args)) {
            int indx = (int)args[1].content.value;
            if (indx > 0)
                // Don't allow negaitve indexing.
                return (BAD);
            if (-indx >= (int)strlen(str1))
                return (BAD);
            res->type = TYP_STRING;
            res->content.string = str1 - indx;
            return (OK);
        }
        return (BAD);
    }

    if (is_zlist(0, args) || is_zlist(1, args))
        return (zlist_funcs::PTminusZ(res, args, datap));

    return (BAD);
}


bool
math_funcs::PTtimes(Variable *res, Variable *args, void *datap)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value = args[0].content.value * args[1].content.value;
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = args[0].content.value * real(args[1]);
            cx->imag = args[0].content.value * imag(args[1]);
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        if (is_scalar(1, args)) {
            cx->real = real(args[0]) * args[1].content.value;
            cx->imag = imag(args[0]) * args[1].content.value;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            cx->real =
                (real(args[0])*real(args[1])) - (imag(args[0])*imag(args[1]));
            cx->imag =
                (real(args[0])*imag(args[1])) + (imag(args[0])*real(args[1]));
            return (OK);
        }
    }

    if (is_zlist(0, args) || is_zlist(1, args))
        return (zlist_funcs::PTandZ(res, args, datap));

    return (BAD);
}


bool
math_funcs::PTmod(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            init_fpe();
            res->content.value =
                fmod(args[0].content.value, args[1].content.value);
            res->type = TYP_SCALAR;
            if (check_fpe("%"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            init_fpe();
            Cmplx *cx = cmplx(res);
            siCmplx v1(args[1]);
            double r1 = fabs(args[0].content.value);
            double r2 = v1.mag();
            double r3 = floor(r1/r2);
            cx->real = args[0].content.value - r3*v1.real;
            cx->imag = -r3*v1.imag;
            if (check_fpe("%"))
                return (BAD);
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        siCmplx v0(args[0]);
        if (is_scalar(1, args)) {
            init_fpe();
            double r1 = v0.mag();
            double r2 = fabs(args[1].content.value);
            double r3 = floor(r1/r2);
            cx->real = v0.real - r3*args[1].content.value;
            cx->imag = v0.imag;
            if (check_fpe("%"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            init_fpe();
            siCmplx v1(args[1]);
            double r1 = v0.mag();
            double r2 = v1.mag();
            double r3 = floor(r1/r2);
            cx->real = v0.real - r3*v1.real;
            cx->imag = v0.imag - r3*v1.imag;
            if (check_fpe("%"))
                return (BAD);
            return (OK);
        }
    }

    return (BAD);
}


bool
math_funcs::PTdivide(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            init_fpe();
            res->content.value = args[0].content.value / args[1].content.value;
            res->type = TYP_SCALAR;
            if (check_fpe("/"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            init_fpe();
            Cmplx *cx = cmplx(res);
            double d = args[0].content.value /
                (real(args[1])*real(args[1]) + imag(args[1])*imag(args[1]));
            cx->real = d*real(args[1]);
            cx->imag = -d*imag(args[1]);
            if (check_fpe("/"))
                return (BAD);
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        if (is_scalar(1, args)) {
            init_fpe();
            cx->real = real(args[0]) / args[1].content.value;
            cx->imag = imag(args[0]) / args[1].content.value;
            if (check_fpe("/"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            init_fpe();
            double d =
                real(args[1])*real(args[1]) + imag(args[1])*imag(args[1]);
            cx->real =
                (real(args[0])*real(args[1]) + imag(args[0])*imag(args[1]))/d;
            cx->imag =
                (imag(args[0])*real(args[1]) - real(args[0])*imag(args[1]))/d;
            if (check_fpe("/"))
                return (BAD);
            return (OK);
        }
    }

    return (BAD);
}


bool
math_funcs::PTpower(Variable *res, Variable *args, void *datap)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            init_fpe();
            res->content.value =
                pow(args[0].content.value, args[1].content.value);
            res->type = TYP_SCALAR;
            if (check_fpe("pow"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            init_fpe();
            Cmplx *cx = cmplx(res);
            siCmplx v1(args[1]);
            double d = fabs(args[0].content.value);
            // r = ln(c1)
            siCmplx r(log(d), 0.0);
            // s = c2*ln(c1)
            siCmplx s(r.real*v1.real, r.real*v1.imag);
            // out = exp(s)
            cx->real = exp(s.real);
            if (s.imag != 0.0) {
                cx->imag = cx->real*sin(s.imag);
                cx->real *= cos(s.imag);
            }
            else
                cx->imag = 0.0;
            if (check_fpe("pow"))
                return (BAD);
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        siCmplx v0(args[0]);
        if (is_scalar(1, args)) {
            init_fpe();
            double d = v0.mag();
            // r = ln(c1)
            siCmplx r(log(d),
                v0.imag != 0.0 ? atan2(v0.imag, v0.real) : 0.0);
            // s = c2*ln(c1)
            d = args[1].content.value;
            siCmplx s(r.real*d, r.imag*d);
            // out = exp(s)
            cx->real = exp(s.real);
            if (s.imag != 0.0) {
                cx->imag = cx->real*sin(s.imag);
                cx->real *= cos(s.imag);
            }
            else
                cx->imag = 0.0;
            if (check_fpe("pow"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            init_fpe();
            siCmplx v1(args[1]);
            double d = v0.mag();
            // r = ln(c1)
            siCmplx r(log(d),
                v0.imag != 0.0 ? atan2(v0.imag, v0.real) : 0.0);
            // s = c2*ln(c1) 
            siCmplx s(r.real*v1.real - r.imag*v1.imag,
                r.imag*v1.real + r.real*v1.imag);
            // out = exp(s)
            cx->real = exp(s.real);
            if (s.imag != 0.0) {
                cx->imag = cx->real*sin(s.imag);
                cx->real *= cos(s.imag);
            }
            else
                cx->imag = 0.0;
            if (check_fpe("pow"))
                return (BAD);
            return (OK);
        }
    }

    if (is_zlist(0, args) || is_zlist(1, args))
        return (zlist_funcs::PTxorZ(res, args, datap));

    return (BAD);
}


bool
math_funcs::PTeq(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (args[0].content.value == args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (args[0].content.value == real(args[1]) &&
                0.0 == imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (real(args[0]) == args[1].content.value &&
                imag(args[0]) == 0.0);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (real(args[0]) == real(args[1]) &&
                imag(args[0]) == imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }

    if (is_string(0, args)) {
        char *str1 = args[0].content.string;
        if (is_string(1, args)) {
            char *str2 = args[1].content.string;
            if (!str1 && !str2)
                res->content.value = 1.0;
            else if (!str1 || !str2)
                res->content.value = 0.0;
            else
                res->content.value = strcmp(str1, str2) == 0;
            res->type = TYP_SCALAR;
            return (OK);
        }
        return (BAD);
    }

    // When a handle is mixed with another type, both are treated as
    // booleans.
    if (is_handle(0, args)) {
        if (is_handle(1, args)) {
            int id1 = (int)args[0].content.value;
            int id2 = (int)args[1].content.value;
            res->content.value = id1 == id2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 1, &b2))
        b1 = args[0].istrue();
        res->content.value = b1 == b2;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_handle(1, args)) {
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 0, &b1))
        b2 = args[1].istrue();
        res->content.value = b1 == b2;
        res->type = TYP_SCALAR;
        return (OK);
    }

    if (is_zlist(0, args) && is_scalar(1, args) &&
            (int)args[1].content.value == 0) {
        res->type = TYP_SCALAR;
        res->content.value = (args[0].content.zlist == 0);
        return (OK);
    }
    if (is_zlist(1, args) && is_scalar(0, args) &&
            (int)args[0].content.value == 0) {
        res->type = TYP_SCALAR;
        res->content.value = (args[1].content.zlist == 0);
        return (OK);
    }

    return (BAD);
}


bool
math_funcs::PTgt(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (args[0].content.value > args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (args[0].content.value > real(args[1]) &&
                0.0 > imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (real(args[0]) > args[1].content.value &&
                imag(args[0]) > 0.0);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (real(args[0]) > real(args[1]) &&
                imag(args[0]) > imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }

    if (is_string(0, args)) {
        char *str1 = args[0].content.string;
        if (is_string(1, args)) {
            char *str2 = args[1].content.string;
            if (!str1)
                res->content.value = 0.0;
            else if (!str2)
                res->content.value = 1.0;
            else
                res->content.value = strcmp(str1, str2) > 0;
            res->type = TYP_SCALAR;
            return (OK);
        }
        return (BAD);
    }

    // When a handle is mixed with another type, both are treated as
    // booleans.
    if (is_handle(0, args)) {
        if (is_handle(1, args)) {
            int id1 = (int)args[0].content.value;
            int id2 = (int)args[1].content.value;
            res->content.value = id1 > id2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 1, &b2))
        b1 = args[0].istrue();
        res->content.value = b1 > b2;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_handle(1, args)) {
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 0, &b1))
        b2 = args[1].istrue();
        res->content.value = b1 > b2;
        res->type = TYP_SCALAR;
        return (OK);
    }

    return (BAD);
}


bool
math_funcs::PTlt(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (args[0].content.value < args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (args[0].content.value < real(args[1]) &&
                0.0 < imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (real(args[0]) < args[1].content.value &&
                imag(args[0]) < 0.0);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (real(args[0]) < real(args[1]) &&
                imag(args[0]) < imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }

    if (is_string(0, args)) {
        char *str1 = args[0].content.string;
        if (is_string(1, args)) {
            char *str2 = args[1].content.string;
            if (!str2)
                res->content.value = 0.0;
            else if (!str1)
                res->content.value = 1.0;
            else
                res->content.value = strcmp(str1, str2) < 0;
            res->type = TYP_SCALAR;
            return (OK);
        }
        return (BAD);
    }

    // When a handle is mixed with another type, both are treated as
    // booleans.
    if (is_handle(0, args)) {
        if (is_handle(1, args)) {
            int id1 = (int)args[0].content.value;
            int id2 = (int)args[1].content.value;
            res->content.value = id1 < id2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 1, &b2))
        b1 = args[0].istrue();
        res->content.value = b1 < b2;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_handle(1, args)) {
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 0, &b1))
        b2 = args[1].istrue();
        res->content.value = b1 < b2;
        res->type = TYP_SCALAR;
        return (OK);
    }

    return (BAD);
}


bool
math_funcs::PTge(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (args[0].content.value >= args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (args[0].content.value >= real(args[1]) &&
                0.0 >= imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (real(args[0]) >= args[1].content.value &&
                imag(args[0]) >= 0.0);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (real(args[0]) >= real(args[1]) &&
                imag(args[0]) >= imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }

    if (is_string(0, args)) {
        char *str1 = args[0].content.string;
        if (is_string(1, args)) {
            char *str2 = args[1].content.string;
            if (!str2)
                res->content.value = 1.0;
            else if (!str1)
                res->content.value = 0.0;
            else
                res->content.value = strcmp(str1, str2) >= 0;
            res->type = TYP_SCALAR;
            return (OK);
        }
        return (BAD);
    }

    // When a handle is mixed with another type, both are treated as
    // booleans.
    if (is_handle(0, args)) {
        if (is_handle(1, args)) {
            int id1 = (int)args[0].content.value;
            int id2 = (int)args[1].content.value;
            res->content.value = id1 >= id2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 1, &b2))
        b1 = args[0].istrue();
        res->content.value = b1 >= b2;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_handle(1, args)) {
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 0, &b1))
        b2 = args[1].istrue();
        res->content.value = b1 >= b2;
        res->type = TYP_SCALAR;
        return (OK);
    }

    return (BAD);
}


bool
math_funcs::PTle(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (args[0].content.value <= args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (args[0].content.value <= real(args[1]) &&
                0.0 <= imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (real(args[0]) <= args[1].content.value &&
                imag(args[0]) <= 0.0);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (real(args[0]) <= real(args[1]) &&
                imag(args[0]) <= imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }

    if (is_string(0, args)) {
        char *str1 = args[0].content.string;
        if (is_string(1, args)) {
            char *str2 = args[1].content.string;
            if (!str1)
                res->content.value = 1.0;
            else if (!str2)
                res->content.value = 0.0;
            else
                res->content.value = strcmp(str1, str2) <= 0;
            res->type = TYP_SCALAR;
            return (OK);
        }
        return (BAD);
    }

    // When a handle is mixed with another type, both are treated as
    // booleans.
    if (is_handle(0, args)) {
        if (is_handle(1, args)) {
            int id1 = (int)args[0].content.value;
            int id2 = (int)args[1].content.value;
            res->content.value = id1 <= id2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 1, &b2))
        b1 = args[0].istrue();
        res->content.value = b1 <= b2;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_handle(1, args)) {
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 0, &b1))
        b2 = args[1].istrue();
        res->content.value = b1 <= b2;
        res->type = TYP_SCALAR;
        return (OK);
    }

    return (BAD);
}


bool
math_funcs::PTne(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (args[0].content.value != args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (args[0].content.value != real(args[1]) &&
                0.0 != imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (real(args[0]) <! args[1].content.value &&
                imag(args[0]) != 0.0);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (real(args[0]) != real(args[1]) &&
                imag(args[0]) != imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }

    if (is_string(0, args)) {
        char *str1 = args[0].content.string;
        if (is_string(1, args)) {
            char *str2 = args[1].content.string;
            // overload for string comparison
            if (!str1 && !str2)
                res->content.value = 0.0;
            else if (!str1 || !str2)
                res->content.value = 1.0;
            else
                res->content.value = strcmp(str1, str2) != 0;
            res->type = TYP_SCALAR;
            return (OK);
        }
        return (BAD);
    }

    // When a handle is mixed with another type, both are treated as
    // booleans.
    if (is_handle(0, args)) {
        if (is_handle(1, args)) {
            int id1 = (int)args[0].content.value;
            int id2 = (int)args[1].content.value;
            res->content.value = id1 != id2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 1, &b2))
        b1 = args[0].istrue();
        res->content.value = b1 != b2;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_handle(1, args)) {
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 0, &b1))
        b2 = args[1].istrue();
        res->content.value = b1 != b2;
        res->type = TYP_SCALAR;
        return (OK);
    }

    if (is_zlist(0, args) && is_scalar(1, args) &&
            (int)args[1].content.value == 0) {
        res->type = TYP_SCALAR;
        res->content.value = (args[0].content.zlist != 0);
        return (OK);
    }
    if (is_zlist(1, args) && is_scalar(0, args) &&
            (int)args[0].content.value == 0) {
        res->type = TYP_SCALAR;
        res->content.value = (args[1].content.zlist != 0);
        return (OK);
    }

    return (BAD);
}


bool
math_funcs::PTand(Variable *res, Variable *args, void *datap)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                to_boolean(args[0].content.value) &&
                to_boolean(args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                to_boolean(args[0].content.value) &&
                (to_boolean(real(args[1])) || to_boolean(imag(args[1])));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                (to_boolean(real(args[0])) || to_boolean(imag(args[0]))) &&
                to_boolean(args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                (to_boolean(real(args[0])) || to_boolean(imag(args[0]))) &&
                (to_boolean(real(args[1])) || to_boolean(imag(args[1])));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }

    if (is_zlist(0, args) || is_zlist(1, args))
        return (zlist_funcs::PTandZ(res, args, datap));

    if (is_handle(0, args)) {
        bool b1, b2;
        if (is_handle(1, args)) {
            b1 = args[0].istrue();
            b2 = args[1].istrue();
            res->content.value = b1 && b2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        ARG_CHK(arg_boolean(args, 1, &b2))
        b1 = args[0].istrue();
        res->content.value = b1 && b2;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_handle(1, args)) {
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 0, &b1))
        b2 = args[1].istrue();
        res->content.value = b1 && b2;
        res->type = TYP_SCALAR;
        return (OK);
    }

    return (BAD);
}


bool
math_funcs::PTor(Variable *res, Variable *args, void *datap)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                to_boolean(args[0].content.value) ||
                to_boolean(args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                to_boolean(args[0].content.value) ||
                to_boolean(real(args[1])) || to_boolean(imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                to_boolean(real(args[0])) || to_boolean(imag(args[0])) ||
                to_boolean(args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value =
                to_boolean(real(args[0])) || to_boolean(imag(args[0])) ||
                to_boolean(real(args[1])) || to_boolean(imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }

    if (is_zlist(0, args) || is_zlist(1, args))
        return (zlist_funcs::PTorZ(res, args, datap));

    if (is_handle(0, args)) {
        bool b1, b2;
        if (is_handle(1, args)) {
            b1 = args[0].istrue();
            b2 = args[1].istrue();
            res->content.value = b1 || b2;
            res->type = TYP_SCALAR;
            return (OK);
        }
        ARG_CHK(arg_boolean(args, 1, &b2))
        b1 = args[0].istrue();
        res->content.value = b1 || b2;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_handle(1, args)) {
        bool b1, b2;
        ARG_CHK(arg_boolean(args, 0, &b1))
        b2 = args[1].istrue();
        res->content.value = b1 || b2;
        res->type = TYP_SCALAR;
        return (OK);
    }

    return (BAD);
}


bool
math_funcs::PTuminus(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = -args[0].content.value;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        cx->real = -real(args[0]);
        cx->imag = -imag(args[0]);
    }
    return (BAD);
}


bool
math_funcs::PTnot(Variable *res, Variable *args, void *datap)
{
    if (is_scalar(0, args)) {
        res->content.value = !to_boolean(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        res->content.value =
            !(to_boolean(real(args[0])) || to_boolean(imag(args[0])));
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_string(0, args)) {
        res->content.value = (args[0].content.string == 0);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_zlist(0, args))
        return (zlist_funcs::PTnotZ(res, args, datap));

    if (is_handle(0, args)) {
        res->content.value = !args[0].istrue();
        res->type = TYP_SCALAR;
        return (OK);
    }
    return (BAD);
}


//------------------------------------------------------------------------
// TYP_ZLIST overrides - these are exported
//------------------------------------------------------------------------

// Evaluation function for the minus operator.  Return the first
// region clipped around the second region.  A numeric argument of 0
// represent empty, nonzero represents full.
//
bool
zlist_funcs::PTminusZ(Variable *res, Variable *args, void *datap)
{
    if (!datap)
        return (BAD);
    SIlexprCx *cx = (SIlexprCx*)datap;
    // At least one arg is known to be TYP_ZLIST
    const Zlist *zlist1;
    if (is_scalar(0, args)) {
        if (to_boolean(args[0].content.value))
            zlist1 = cx->getZref();
        else
            zlist1 = 0;
    }
    else if (is_zlist(0, args))
        zlist1 = args[0].content.zlist;
    else
        return (BAD);

    const Zlist *zlist2;
    if (is_scalar(1, args)) {
        if (to_boolean(args[1].content.value))
            zlist2 = cx->getZref();
        else
            zlist2 = 0;
    }
    else if (is_zlist(1, args))
        zlist2 = args[1].content.zlist;
    else
        return (BAD);

    if (cx->verbose())
        SIparse()->ifSendMessage("Performing ANDNOT ...");
    if (SIparse()->ifCheckInterrupt()) {
        cx->handleInterrupt();
        return (OK);
    }

    res->type = TYP_ZLIST;

#ifdef TIMEDBG
    unsigned long t1 = Tvals::millisec();
#endif

    Zlist *z1 = Zlist::copy(zlist1);
    XIrt ret = Zlist::zl_andnot(&z1, Zlist::copy(zlist2));
    if (ret != XIok) {
        if (ret == XIintr) {
            res->content.zlist = 0;
            cx->handleInterrupt();
            return (OK);
        }
        return (BAD);
    }
    res->content.zlist = z1;

#ifdef TIMEDBG
    unsigned long t2 = Tvals::millisec();
    printf("andnot %g\n", (t2-t1)/1000.0);
#endif

    return (OK);
}


// Evaluation for AND operator.  Return the intersection.  A numeric
// argument of 0 represent empty, nonzero represents full.
//
bool
zlist_funcs::PTandZ(Variable *res, Variable *args, void *datap)
{
    if (!datap)
        return (BAD);
    SIlexprCx *cx = (SIlexprCx*)datap;
    // At least one arg is known to be TYP_ZLIST
    const Zlist *zlist1;
    if (is_scalar(0, args)) {
        if (to_boolean(args[0].content.value))
            zlist1 = cx->getZref();
        else
            zlist1 = 0;
    }
    else if (is_zlist(0, args))
        zlist1 = args[0].content.zlist;
    else
        return (BAD);

    const Zlist *zlist2;
    if (is_scalar(1, args)) {
        if (to_boolean(args[1].content.value))
            zlist2 = cx->getZref();
        else
            zlist2 = 0;
    }
    else if (is_zlist(1, args))
        zlist2 = args[1].content.zlist;
    else
        return (BAD);

    if (cx->verbose())
        SIparse()->ifSendMessage("Performing AND ...");
    if (SIparse()->ifCheckInterrupt()) {
        cx->handleInterrupt();
        return (OK);
    }

    res->type = TYP_ZLIST;

#ifdef TIMEDBG
    unsigned long t1 = Tvals::millisec();
#endif

    Zlist *z1 = Zlist::copy(zlist1);
    XIrt ret = Zlist::zl_and(&z1, Zlist::copy(zlist2));
    if (ret != XIok) {
        if (ret == XIintr) {
            res->content.zlist = 0;
            cx->handleInterrupt();
            return (OK);
        }
        return (BAD);
    }
    res->content.zlist = z1;

#ifdef TIMEDBG
    unsigned long t2 = Tvals::millisec();
    printf("and %g\n", (t2-t1)/1000.0);
#endif

    return (OK);
}


// Evaluation for OR operator.  Return the union.  A numeric argument
// of 0 represent empty, nonzero represents full.
//
bool
zlist_funcs::PTorZ(Variable *res, Variable *args, void *datap)
{
    if (!datap)
        return (BAD);
    SIlexprCx *cx = (SIlexprCx*)datap;
    // At least one arg is known to be TYP_ZLIST
    const Zlist *zlist1;
    if (is_scalar(0, args)) {
        if (to_boolean(args[0].content.value))
            zlist1 = cx->getZref();
        else
            zlist1 = 0;
    }
    else if (is_zlist(0, args))
        zlist1 = args[0].content.zlist;
    else
        return (BAD);

    const Zlist *zlist2;
    if (is_scalar(1, args)) {
        if (to_boolean(args[1].content.value))
            zlist2 = cx->getZref();
        else
            zlist2 = 0;
    }
    else if (is_zlist(1, args))
        zlist2 = args[1].content.zlist;
    else
        return (BAD);

    if (cx->verbose())
        SIparse()->ifSendMessage("Performing OR ...");
    if (SIparse()->ifCheckInterrupt()) {
        cx->handleInterrupt();
        return (OK);
    }

    res->type = TYP_ZLIST;

#ifdef TIMEDBG
    unsigned long t1 = Tvals::millisec();
#endif

    Zlist *z1 = Zlist::copy(zlist1);
    XIrt ret = Zlist::zl_or(&z1, Zlist::copy(zlist2));
    if (ret != XIok) {
        if (ret == XIintr) {
            res->content.zlist = 0;
            cx->handleInterrupt();
            return (OK);
        }
        return (BAD);
    }
    res->content.zlist = z1;

#ifdef TIMEDBG
    unsigned long t2 = Tvals::millisec();
    printf("or %g\n", (t2-t1)/1000.0);
#endif

    return (OK);
}


// Evaluation function for the XOR operator.  Return the exclusive-or
// of the two regions.  A numeric argument of 0 represent empty,
// nonzero represents full.
//
bool
zlist_funcs::PTxorZ(Variable *res, Variable *args, void *datap)
{
    if (!datap)
        return (BAD);
    SIlexprCx *cx = (SIlexprCx*)datap;
    // At least one arg is known to be TYP_ZLIST
    const Zlist *zlist1;
    if (is_scalar(0, args)) {
        if (to_boolean(args[0].content.value))
            zlist1 = cx->getZref();
        else
            zlist1 = 0;
    }
    else if (is_zlist(0, args))
        zlist1 = args[0].content.zlist;
    else
        return (BAD);

    const Zlist *zlist2;
    if (is_scalar(1, args)) {
        if (to_boolean(args[1].content.value))
            zlist2 = cx->getZref();
        else
            zlist2 = 0;
    }
    else if (is_zlist(1, args))
        zlist2 = args[1].content.zlist;
    else
        return (BAD);

    if (cx->verbose())
        SIparse()->ifSendMessage("Performing XOR ...");
    if (SIparse()->ifCheckInterrupt()) {
        cx->handleInterrupt();
        return (OK);
    }

    res->type = TYP_ZLIST;

#ifdef TIMEDBG
    unsigned long t1 = Tvals::millisec();
#endif

    Zlist *z1 = Zlist::copy(zlist1);
    Zlist *z2 = Zlist::copy(zlist2);
    XIrt ret = Zlist::zl_xor(&z1, z2);
    if (ret != XIok) {
        if (ret == XIintr) {
            res->content.zlist = 0;
            cx->handleInterrupt();
            return (OK);
        }
        return (BAD);
    }
    res->content.zlist = z1;

#ifdef TIMEDBG
    unsigned long t2 = Tvals::millisec();
    printf("xor %g\n", (t2-t1)/1000.0);
#endif

    return (OK);
}


// Evaluation for NOT operator.
//
bool
zlist_funcs::PTnotZ(Variable *res, Variable *args, void *datap)
{
    if (!datap)
        return (BAD);
    SIlexprCx *cx = (SIlexprCx*)datap;
    // arg is known to be TYP_ZLIST
    const Zlist *zlist1 = cx->getZref();

    const Zlist *zlist2;
    if (is_scalar(0, args)) {
        if (to_boolean(args[0].content.value))
            zlist2 = cx->getZref();
        else
            zlist2 = 0;
    }
    else if (is_zlist(0, args))
        zlist2 = args[0].content.zlist;
    else
        return (BAD);

    if (cx->verbose())
        SIparse()->ifSendMessage("Performing NOT ...");
    if (SIparse()->ifCheckInterrupt()) {
        cx->handleInterrupt();
        return (OK);
    }

    res->type = TYP_ZLIST;

#ifdef TIMEDBG
    unsigned long t1 = Tvals::millisec();
#endif

    Zlist *z1 = Zlist::copy(zlist1);
    XIrt ret = Zlist::zl_andnot(&z1, Zlist::copy(zlist2));
    if (ret != XIok) {
        if (ret == XIintr) {
            res->content.zlist = 0;
            cx->handleInterrupt();
            return (OK);
        }
        return (BAD);
    }
    res->content.zlist = z1;

#ifdef TIMEDBG
    unsigned long t2 = Tvals::millisec();
    printf("not %g\n", (t2-t1)/1000.0);
#endif

    return (OK);
}


//------------------------------------------------------------------------
// Complex numbers
//------------------------------------------------------------------------

bool
math_funcs::PTcmplx(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        Cmplx *cx = cmplx(res);
        cx->real = args[0].content.value;
        if (is_scalar(1, args)) {
            cx->imag = args[1].content.value;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            cx->imag = args[1].content.cx.real;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        cx->real = args[0].content.cx.real;
        if (is_scalar(1, args)) {
            cx->imag = args[1].content.value;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            cx->imag = args[1].content.cx.real;
            return (OK);
        }
    }
    return (BAD);
}


bool
math_funcs::PTreal(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = args[0].content.value;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        res->content.value = real(args[0]);
        res->type = TYP_SCALAR;
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTimag(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = 0.0;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        res->content.value = imag(args[0]);
        res->type = TYP_SCALAR;
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTmag(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = fabs(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        res->content.value = args[0].content.cx.mag();
        res->type = TYP_SCALAR;
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTang(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = 0.0;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        res->content.value = atan2(imag(args[0]), real(args[0]));
        res->type = TYP_SCALAR;
        return (OK);
    }
    return (BAD);
}


//------------------------------------------------------------------------
// Math functions
//------------------------------------------------------------------------

bool
math_funcs::PTabs(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = fabs(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        res->content.value = args[0].content.cx.mag();
        res->type = TYP_SCALAR;
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTsgn(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = args[0].content.value > 0.0 ? 1.0 :
            args[0].content.value < 0.0 ? -1.0 : 0.0;
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        res->content.cx.real = args[0].content.cx.real > 0.0 ? 1.0 :
            args[0].content.cx.real < 0.0 ? -1.0 : 0.0;
        res->content.cx.imag = args[0].content.cx.imag > 0.0 ? 1.0 :
            args[0].content.cx.imag < 0.0 ? -1.0 : 0.0;
        res->type = TYP_CMPLX;
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTacos(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = acos(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("acos"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        // -i*ln(z + sqrt(z*z-1))
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        siCmplx c1(v.real*v.real - v.imag*v.imag - 1, 2*v.real*v.imag);
        c1.csqrt();
        c1.real += v.real;
        c1.imag += v.imag;
        cx->real = atan2(c1.imag, c1.real);
        cx->imag = -log(c1.mag());
        if (check_fpe("acos"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTasin(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = asin(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("asin"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        // pi/2 - i*ln(z + sqrt(z*z-1))
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        siCmplx c1(v.real*v.real - v.imag*v.imag - 1, 2*v.real*v.imag);
        c1.csqrt();
        c1.real += v.real;
        c1.imag += v.imag;
        cx->real = M_PI_2 + atan2(c1.imag, c1.real);
        cx->imag = -log(c1.mag());
        if (check_fpe("asin"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTatan(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = atan(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        // 1/2i * ln( (1+iz)/(1-iz) )
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        siCmplx c2(1.0 + v.imag, -v.real);
        double d = c2.real*c2.real + c2.imag*c2.imag;
        siCmplx c1(1.0 - v.imag, v.real);
        siCmplx c3(c1.real*c2.real + c1.imag*c2.imag,
            c1.imag*c2.real - c2.imag*c1.real);
        d = c3.mag()/d;
        cx->real = 0.5*atan2(c3.imag, c3.real);
        cx->imag = -0.5*log(d);
        return (OK);
    }
    return (BAD);
}


#ifdef HAVE_ACOSH

bool
math_funcs::PTacosh(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = acosh(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("acosh"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        // -i * acos(z)
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        siCmplx c1(v.real*v.real - v.imag*v.imag - 1, 2*v.real*v.imag);
        c1.csqrt();
        c1.real += v.real;
        c1.imag += v.imag;
        cx->real = log(c1.mag());
        cx->imag = atan2(c1.imag, c1.real);
        if (check_fpe("acosh"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTasinh(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = asinh(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("asinh"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        // -i * asin(i*z)
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        siCmplx c0(-v.imag, v.real);
        siCmplx c1(c0.real*c0.real - c0.imag*c0.imag - 1, 2*c0.real*c0.imag);
        c1.csqrt();
        c1.real += c0.real;
        c1.imag += c0.imag;
        cx->real = -log(c1.mag());
        cx->imag = -M_PI_2 + atan2(c1.imag, c1.real);
        if (check_fpe("asinh"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTatanh(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = atanh(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("atanh"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        // -i * atan(i*z) -> -1/2 ln( (1-z)/(1+z) )
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        siCmplx c2(1.0 + v.real, v.imag);
        double d = c2.real*c2.real + c2.imag*c2.imag;
        siCmplx c1(1.0 - v.real, -v.imag);
        siCmplx c3(c1.real*c2.real + c1.imag*c2.imag,
            c1.imag*c2.real - c2.imag*c1.real);
        d = c3.mag()/d;
        cx->real = -0.5*log(d);
        cx->imag = -0.5*atan2(c3.imag, c3.real);
        if (check_fpe("atanh"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}

#endif // HAVE_ACOSH


bool
math_funcs::PTcbrt(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = cbrt(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        double d = v.mag();
        if (d > 0) {
            siCmplx r(log(d)/3.0,
                v.imag != 0.0 ? atan2(v.imag, v.real)/3.0 : 0.0);
            cx->real = exp(r.real);
            if (r.imag != 0.0) {
                cx->imag = cx->real*sin(r.imag);
                cx->real *= cos(r.imag);
            }
        }
        else {
            cx->real = 0.0;
            cx->imag = 0.0;
        }
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTcos(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = cos(MODULUS(args[0].content.value, M_PI+M_PI));
        res->type = TYP_SCALAR;
        if (check_fpe("cos"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        cx->real = cos(v.real) * cosh(v.imag);
        cx->imag = -sin(v.real) * sinh(v.imag);
        if (check_fpe("cos"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTcosh(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = cosh(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("cosh"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        cx->real = cosh(v.real) * cos(v.imag);
        cx->imag = sinh(v.real) * sin(v.imag);
        if (check_fpe("cosh"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTj0(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        errno = 0;
        res->content.value = j0(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_rng("j0"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        errno = 0;
        res->content.value = j0(args[0].content.cx.real);
        res->type = TYP_SCALAR;
        if (check_rng("j0"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTj1(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        errno = 0;
        res->content.value = j1(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_rng("j1"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        errno = 0;
        res->content.value = j1(args[0].content.cx.real);
        res->type = TYP_SCALAR;
        if (check_rng("j1"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTjn(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            errno = 0;
            res->content.value =
                jn(mmRnd(args[0].content.value), args[1].content.value);
            res->type = TYP_SCALAR;
            if (check_rng("jn"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            errno = 0;
            res->content.value =
                jn(mmRnd(args[0].content.value), args[1].content.cx.real);
            res->type = TYP_SCALAR;
            if (check_rng("jn"))
                return (BAD);
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            errno = 0;
            res->content.value =
                jn(mmRnd(args[0].content.cx.real), args[1].content.value);
            res->type = TYP_SCALAR;
            if (check_rng("jn"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            errno = 0;
            res->content.value =
                jn(mmRnd(args[0].content.cx.real), args[1].content.cx.real);
            res->type = TYP_SCALAR;
            if (check_rng("jn"))
                return (BAD);
            return (OK);
        }
    }
    return (BAD);
}


bool
math_funcs::PTy0(Variable *res, Variable *args, void*)
{
    // Yes, the y's do need both the fenv and errno tests,
    // passing 0 gives a range error but no divide-by-zero.
    if (is_scalar(0, args)) {
        errno = 0;
        init_fpe();
        res->content.value = y0(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("y0"))
            return (BAD);
        if (check_rng("y0"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        errno = 0;
        init_fpe();
        res->content.value = y0(args[0].content.cx.real);
        res->type = TYP_SCALAR;
        if (check_fpe("y0"))
            return (BAD);
        if (check_rng("y0"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTy1(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        errno = 0;
        init_fpe();
        res->content.value = y1(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("y1"))
            return (BAD);
        if (check_rng("y1"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        errno = 0;
        init_fpe();
        res->content.value = y1(args[0].content.cx.real);
        res->type = TYP_SCALAR;
        if (check_fpe("y1"))
            return (BAD);
        if (check_rng("y1"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTyn(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            errno = 0;
            init_fpe();
            res->content.value =
                yn(mmRnd(args[0].content.value), args[1].content.value);
            res->type = TYP_SCALAR;
            if (check_fpe("yn"))
                return (BAD);
            if (check_rng("yn"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            errno = 0;
            init_fpe();
            res->content.value =
                yn(mmRnd(args[0].content.value), args[1].content.cx.real);
            res->type = TYP_SCALAR;
            if (check_fpe("yn"))
                return (BAD);
            if (check_rng("yn"))
                return (BAD);
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            errno = 0;
            init_fpe();
            res->content.value =
                yn(mmRnd(args[0].content.cx.real), args[1].content.value);
            res->type = TYP_SCALAR;
            if (check_fpe("yn"))
                return (BAD);
            if (check_rng("yn"))
                return (BAD);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            errno = 0;
            init_fpe();
            res->content.value =
                yn(mmRnd(args[0].content.cx.real), args[1].content.cx.real);
            res->type = TYP_SCALAR;
            if (check_fpe("yn"))
                return (BAD);
            if (check_rng("yn"))
                return (BAD);
            return (OK);
        }
    }
    return (BAD);
}


bool
math_funcs::PTerf(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = erf(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("erf"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        res->content.value = erf(args[0].content.cx.real);
        res->type = TYP_SCALAR;
        if (check_fpe("erf"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTerfc(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = erfc(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("erfc"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        res->content.value = erfc(args[0].content.cx.real);
        res->type = TYP_SCALAR;
        if (check_fpe("erfc"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTexp(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = exp(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("exp"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        double d = exp(v.real);
        cx->real = d*cos(v.imag);
        cx->imag = d*sin(v.imag);
        if (check_fpe("exp"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTln(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        double d = args[0].content.value;
        if (d < TINYNUM)
            res->content.value = log(TINYNUM);
        else
            res->content.value = log(d);
        res->type = TYP_SCALAR;
        if (check_fpe("ln"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        double d = v.mag();
        if (d < TINYNUM)
            cx->real = log(TINYNUM);
        else
            cx->real = log(d);
        cx->imag = atan2(v.imag, v.real);
        if (check_fpe("ln"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTlog(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        double d = args[0].content.value;
        if (SIinterp::LogIsLog10()) {
            if (d < TINYNUM)
                res->content.value = log10(TINYNUM);
            else
                res->content.value = log10(d);
        }
        else {
            if (d < TINYNUM)
                res->content.value = log(TINYNUM);
            else
                res->content.value = log(d);
        }
        if (check_fpe("log"))
            return (BAD);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        double d = v.mag();
        if (SIinterp::LogIsLog10()) {
            if (d < TINYNUM)
                cx->real = log10(TINYNUM);
            else
                cx->real = log10(d);
            cx->imag = M_LOG10E*atan2(v.imag, v.real);
        }
        else {
            if (d < TINYNUM)
                cx->real = log(TINYNUM);
            else
                cx->real = log(d);
            cx->imag = atan2(v.imag, v.real);
        }
        if (check_fpe("log"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTlog10(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        double d = args[0].content.value;
        if (d < TINYNUM)
            res->content.value = log10(TINYNUM);
        else
            res->content.value = log10(d);
        res->type = TYP_SCALAR;
        if (check_fpe("log10"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        double d = v.mag();
        if (d < TINYNUM)
            cx->real = log10(TINYNUM);
        else
            cx->real = log10(d);
        cx->imag = M_LOG10E*atan2(v.imag, v.real);
        if (check_fpe("log10"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTsin(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = sin(MODULUS(args[0].content.value, M_PI+M_PI));
        res->type = TYP_SCALAR;
        if (check_fpe("sin"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        cx->real = sin(v.real) * cosh(v.imag);
        cx->imag = cos(v.real) * sinh(v.imag);
        if (check_fpe("sin"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTsinh(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = sinh(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("sinh"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        cx->real = sinh(v.real) * cos(v.imag);
        cx->imag = cosh(v.real) * sin(v.imag);
        if (check_fpe("sinh"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTsqrt(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = sqrt(args[0].content.value);
        res->type = TYP_SCALAR;
        if (check_fpe("sqrt"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        *cx = args[0].content.cx;
        cx->csqrt();
        if (check_fpe("sqrt"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTtan(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        init_fpe();
        res->content.value = tan(MODULUS(args[0].content.value, M_PI+M_PI));
        res->type = TYP_SCALAR;
        if (check_fpe("tan"))
            return (BAD);
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        double su = sin(v.real);
        double cu = cos(v.real);
        double shv = sinh(v.imag);
        double chv = cosh(v.imag);
        siCmplx c1(su*chv, cu*shv);
        siCmplx c2(cu*chv, -su*shv);   
        double d = c2.real*c2.real + c2.imag*c2.imag;
        cx->real = (c1.real*c2.real + c1.imag*c2.imag)/d;
        cx->imag = (c1.imag*c2.real - c2.imag*c1.real)/d;
        if (check_fpe("tan"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTtanh(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = tanh(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        init_fpe();
        Cmplx *cx = cmplx(res);
        siCmplx v(args[0]);
        double shu = sinh(v.real);
        double chu = cosh(v.real);
        double sv = sin(v.imag);  
        double cv = cos(v.imag);
        siCmplx c1(shu*cv, chu*sv);
        siCmplx c2(chu*cv, shu*sv);   
        double d = c2.real*c2.real + c2.imag*c2.imag;
        cx->real = (c1.real*c2.real + c1.imag*c2.imag)/d;
        cx->imag = (c1.imag*c2.real - c2.imag*c1.real)/d;
        if (check_fpe("tanh"))
            return (BAD);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTatan2(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value = atan2(args[0].content.value,
                args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value = atan2(args[0].content.value,
                args[1].content.cx.real);
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value = atan2(args[0].content.cx.real,
                args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            res->content.value = atan2(args[0].content.cx.real,
                args[1].content.cx.real);
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    return (BAD);
}


namespace {
    sRnd rnd;
    bool seeded;
}


// This function applies a seed to the random number generators.  This
// can be used to ensure that successive runs using random numbers
// choose different values.  The seed value is converted to an integer.
//
bool
math_funcs::PTseed(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        rnd.seed((unsigned int)args[0].content.value);
        res->type = TYP_SCALAR;
        res->content.value = 1;
        seeded = true;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        rnd.seed((unsigned int)args[0].content.cx.real);
        res->type = TYP_SCALAR;
        res->content.value = 1;
        seeded = true;
        return (OK);
    }
    return (BAD);
}


// This returns a random value in the range [0-1), which can
// optionally be seeded with the Seed() function.  The numbers
// generated have a uniform distribution.
//
bool
math_funcs::PTrandom(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    if (!seeded) {
        rnd.seed(time(0));
        seeded = true;
    }
    res->content.value = rnd.random();
    return (OK);
}


// This returns Gaussian random numbers with zero mean and unit deviation.
//
bool
math_funcs::PTgauss(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    if (!seeded) {
        rnd.seed(time(0));
        seeded = true;
    }
    res->content.value = rnd.gauss();
    return (OK);
}


bool
math_funcs::PTfloor(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = floor(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        cx->real = floor(args[0].content.cx.real);
        cx->imag = floor(args[0].content.cx.imag);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTceil(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = ceil(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        cx->real = ceil(args[0].content.cx.real);
        cx->imag = ceil(args[0].content.cx.imag);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTrint(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = mmRnd(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        cx->real = mmRnd(args[0].content.cx.real);
        cx->imag = mmRnd(args[0].content.cx.imag);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTint(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        res->content.value = (int)(args[0].content.value);
        res->type = TYP_SCALAR;
        return (OK);
    }
    if (is_cmplx(0, args)) {
        Cmplx *cx = cmplx(res);
        cx->real = (int)(args[0].content.cx.real);
        cx->imag = (int)(args[0].content.cx.imag);
        return (OK);
    }
    return (BAD);
}


bool
math_funcs::PTmax(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                mmMax(args[0].content.value, args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = mmMax(args[0].content.value, real(args[1]));
            cx->imag = mmMax(0.0, imag(args[1]));
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = mmMax(real(args[0]), args[1].content.value);
            cx->imag = mmMax(real(args[0]), 0.0);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = mmMax(real(args[0]), real(args[1]));
            cx->imag = mmMax(imag(args[0]), imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    return (BAD);
}


bool
math_funcs::PTmin(Variable *res, Variable *args, void*)
{
    if (is_scalar(0, args)) {
        if (is_scalar(1, args)) {
            res->content.value =
                mmMin(args[0].content.value, args[1].content.value);
            res->type = TYP_SCALAR;
            return (OK);
        }
        if (is_cmplx(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = mmMin(args[0].content.value, real(args[1]));
            cx->imag = mmMin(0.0, imag(args[1]));
            return (OK);
        }
    }
    else if (is_cmplx(0, args)) {
        if (is_scalar(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = mmMin(real(args[0]), args[1].content.value);
            cx->imag = mmMin(real(args[0]), 0.0);
            return (OK);
        }
        if (is_cmplx(1, args)) {
            Cmplx *cx = cmplx(res);
            cx->real = mmMin(real(args[0]), real(args[1]));
            cx->imag = mmMin(imag(args[0]), imag(args[1]));
            res->type = TYP_SCALAR;
            return (OK);
        }
    }
    return (BAD);
}

