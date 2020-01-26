
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
Authors: 1987 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef INPPTREE_H
#define INPPTREE_H

#include "inptran.h"

typedef struct IFparseNode ParseNode;
#include "spnumber/spparse.h"


//
// Definitions for the expression parser.
//
// Note that IFparseTree is not defined here, but in ifdata.h, which
// is exported to the device library.  This file is not visible in the
// device library.
//

// Enable relational operators in source functions.  These can cause
// trouble as they have singular derivatives, but can be very useful.
#define NEWOPS

// These are the possible types of nodes we can have in the parse tree.
//
enum PTtype
{
    PT_PLACEHOLDER = TT_END,
    PT_PLUS = TT_PLUS,
    PT_MINUS = TT_MINUS,
    PT_TIMES = TT_TIMES,
    PT_DIVIDE = TT_DIVIDE,
    PT_POWER = TT_POWER,
    PT_COMMA = TT_COMMA,

#ifdef NEWOPS
    PT_EQ = TT_EQ,
    PT_GT = TT_GT,
    PT_LT = TT_LT,
    PT_GE = TT_GE,
    PT_LE = TT_LE,
    PT_NE = TT_NE,
    PT_AND = TT_AND,
    PT_OR = TT_OR,
    PT_NOT = TT_NOT,
#endif

    PT_FUNCTION,
    PT_CONSTANT,
    PT_VAR,
    PT_PARAM,
    PT_TFUNC,
    PT_MACROARG,
    PT_MACRO,
    PT_MACRO_DERIV
};

// These are the math functions that we support.
//
enum PTfType
{
    PTF_NIL,
    PTF_ABS,
    PTF_ACOS,
    PTF_ACOSH,
    PTF_ASIN,
    PTF_ASINH,
    PTF_ATAN,
    PTF_ATANH,
    PTF_CBRT,
    PTF_COS,
    PTF_COSH,
    PTF_DERIV,
    PTF_ERF,
    PTF_ERFC,
    PTF_EXP,
    PTF_J0,
    PTF_J1,
    PTF_JN,
    PTF_LN,
    PTF_LOG10,
    PTF_POW,
    PTF_SGN,
    PTF_SIN,
    PTF_SINH,
    PTF_SQRT,
    PTF_TAN,
    PTF_TANH,
    PTF_UMINUS,
    PTF_Y0,
    PTF_Y1,
    PTF_YN
};


struct IFparseTree;
struct IFparseNode;
struct IFmacro;
struct IFmacroDeriv;
struct sLstr;
struct sCKT;
struct sCKTtable;

typedef double(IFparseNode::*PTfuncType)(const double*);

// Structure:   IFparseNode
//
// Representation of a node for the parse tree struct below.
//
struct IFparseNode
{
    friend struct PTelement;
    friend struct IFparseTree;
    friend struct IFmacro;
    friend struct sCKT;
#define NEWTF
#ifdef NEWTF
    friend void IFpulseData::parse(const char*, IFparseNode*, int*);
    friend void IFgpulseData::parse(const char*, IFparseNode*, int*);
    friend void IFpwlData::parse(const char*, IFparseNode*, int*);
    friend void IFsinData::parse(const char*, IFparseNode*, int*);
    friend void IFspulseData::parse(const char*, IFparseNode*, int*);
    friend void IFexpData::parse(const char*, IFparseNode*, int*);
    friend void IFsffmData::parse(const char*, IFparseNode*, int*);
    friend void IFamData::parse(const char*, IFparseNode*, int*);
    friend void IFgaussData::parse(const char*, IFparseNode*, int*);
    friend void IFinterpData::parse(const char*, IFparseNode*, int*);
#endif

    struct PTop
    {
        PTop(PTtype t, const char *n, PTfuncType f)
            {
                number = t;
                name = n;
                funcptr = f;
            }

        PTtype number;
        const char *name;
        PTfuncType funcptr;
    };
    static PTop PTops[];

    struct PTfunc
    {
        PTfunc(const char *n, PTfType t, PTfuncType f)
            {
                name = n;
                number = t;
                funcptr = f;
            }

        const char *name;
        PTfType number;
        PTfuncType funcptr;
    };
    static PTfunc PTfuncs[];

    struct PTtfunc
    {
        PTtfunc(const char *n, PTftType t, PTfuncType f)
            {
                name = n;
                number = t;
                funcptr = f;
            }

        const char *name;
        PTftType number;
        PTfuncType funcptr;
    };
    static PTtfunc PTtFuncs[];

    typedef int(IFparseNode::*PevalFuncType)(double*, const double*,
        const double*);

    IFparseNode()
        {
            p_left = 0;
            p_right = 0;
            p_tree = 0;
            p_evfunc = 0;

            p_valname = 0;
            p_type = PT_PLACEHOLDER;
            p_valindx = 0;
            p_newderiv = 0;
            p_initialized = false;
            p_func = 0;
            p_auxval = 0.0;
            p_auxtime = 0.0;
            memset(&v, 0, sizeof(v));
        }

    ~IFparseNode();

    static IFparseNode *copy(IFparseNode *p, bool skip_nd = false)
        {
            return (p ? p->copy_prv(skip_nd) : 0);
        }

    static bool check(const IFparseNode*);
    static bool check_macro(const IFparseNode*);
    static bool is_const(const IFparseNode*);
    void collapse(IFparseNode**);
    void set_args(const char*);
    double time_limit(sCKT*, double);
    void print_string(sLstr&, TokenType = TT_END, bool = false);
    char *get_string(int = 0);

    static bool parse_if_tranfunc(IFparseNode*, const char*, int*);

    PTtype type()               { return ((PTtype)p_type); }
    IFparseNode *left()         { return (p_left); }
    IFparseNode *right()        { return (p_right); }
    IFtranData *tranData()      { return (p_type == PT_TFUNC ? v.td : 0); }

private:
    // evaluation functions
    int p_const(double*, const double*, const double*);
    int p_var(double*, const double*, const double*);
    int p_parm(double*, const double*, const double*);
    int p_vlparm(double*, const double*, const double*);
    int p_op(double*, const double*, const double*);
    int p_fcn(double*, const double*, const double*);
    int p_tran(double*, const double*, const double*);
    int p_table(double*, const double*, const double*);
    int p_macarg(double*, const double*, const double*);
    int p_macro(double*, const double*, const double*);
    int p_macro_deriv(double*, const double*, const double*);

    IFparseNode *copy_prv(bool = false);
    void p_init_node(double, double);
    void p_init_func(double, double, bool = false);

    // operations
    double PTplus(const double*);
    double PTminus(const double*);
    double PTtimes(const double*);
    double PTdivide(const double*);
    double PTpower(const double*);

#ifdef NEWOPS
    double PTeq(const double*);
    double PTgt(const double*);
    double PTlt(const double*);
    double PTge(const double*);
    double PTle(const double*);
    double PTne(const double*);
    double PTand(const double*);
    double PTor(const double*);
    double PTnot(const double*);
#endif

    bool p_two_args(int);

    // functions
    double PTabs(const double*);
    double PTsgn(const double*);
    double PTacos(const double*);
    double PTasin(const double*);
    double PTatan(const double*);
#ifdef HAVE_ACOSH
    double PTacosh(const double*);
    double PTasinh(const double*);
    double PTatanh(const double*);
#endif
    double PTcbrt(const double*);
    double PTcos(const double*);
    double PTcosh(const double*);
    double PTerf(const double*);
    double PTerfc(const double*);
    double PTexp(const double*);
    double PTj0(const double*);
    double PTj1(const double*);
    double PTjn(const double*);
    double PTln(const double*);
    double PTlog10(const double*);
    double PTpow(const double*);
    double PTsin(const double*);
    double PTsinh(const double*);
    double PTsqrt(const double*);
    double PTtan(const double*);
    double PTtanh(const double*);
    double PTuminus(const double*);
    double PTy0(const double*);
    double PTy1(const double*);
    double PTyn(const double*);

    // "tran" funcs
    double tran_func(const double*);
    double tran_deriv(const double*);
    double PTtTable(const double*);
    double PTtTableD(const double*);

    IFparseNode *p_left;        // Left operand, or single operand
    IFparseNode *p_right;       // Right operand, if there is one
    IFparseTree *p_tree;        // Top tree;
    PevalFuncType p_evfunc;     // called during evaluation

    const char *p_valname;      // name of function or parameter
    short int p_type;           // type of node (PTtype)
    short int p_valindx;        // one of PTF_* or other index
    bool p_newderiv;            // true if created after parse
    bool p_initialized;         // true if tranfunc was initialized
    PTfuncType p_func;          // if math or tran func or oper
    double p_auxval;            // store last value for Verilog
    double p_auxtime;           // store last time for Verilog
    union {
        double constant;        // if CONSTANT
        sCKTtable *table;       // if table
        IFspecial *sp;          // if special
        IFtranData *td;         // if tran func
        IFmacro *macro;         // if macro
        IFmacroDeriv *macro_deriv; // if macro derivative
    } v;
};


// The parser stack element.
//
struct PTelement : public Element
{
    friend int EvalTranFunc(double**, const char*, double*, int);

    IFparseNode *makeNode(void*);
    IFparseNode *makeBnode(IFparseNode*, IFparseNode*, void*);
    IFparseNode *makeFnode(IFparseNode*, void*);
    IFparseNode *makeUnode(IFparseNode*, void*);
    IFparseNode *makeSnode(void*);
    IFparseNode *makeNnode(void*);
    char *userString(const char**, bool);
};

// Hook to User-Defined Functions.
//
struct IFmacro
{
    IFmacro(const char*, int, const char*, const char*, sCKT*);
    ~IFmacro();

    int set_args(IFparseNode*, const double*, const double*, double*);
    int evaluate(double*, const double*, const double*);

    const char *name()      const { return (m_name); }
    int numargs()           const { return (m_numargs); }
    IFparseTree *tree()     { return (m_tree); }
    const char *text()      const { return (m_text); }
    const char *argnames()  const { return (m_argnames); }
    bool has_tranfunc()     const { return (m_has_tranfunc); }

private:
    IFparseTree *m_tree;
    char *m_name;
    char *m_text;
    char *m_argnames;
    int m_numargs;
    bool m_has_tranfunc;
};


// After differentiation, this structure is used to call a differentiated
// macro;
//
struct IFmacroDeriv
{
    IFmacroDeriv(IFmacro *m)
        {
            md_macro = m;
            md_argdrvs = new IFparseNode*[m->numargs()];
            memset(md_argdrvs, 0, m->numargs()*sizeof(IFparseNode*));
        }

    // This does not copy the arg derivs.
    IFmacroDeriv(IFmacroDeriv &md)
        {
            md_macro = md.md_macro;
            md_argdrvs = new IFparseNode*[md_macro->numargs()];
            memset(md_argdrvs, 0, md_macro->numargs()*sizeof(IFparseNode*));
        }

    ~IFmacroDeriv()
        {
            delete [] md_argdrvs;
        }

    IFmacro *md_macro;
    IFparseNode **md_argdrvs;
};

#endif

