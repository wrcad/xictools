
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: inpptree.h,v 2.60 2016/03/16 02:50:28 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef INPPTREE_H
#define INPPTREE_H

typedef struct IFparseNode ParseNode;
#include "spparse.h"


//
// Definitions for the expression parser.
//
// Note that IFparseTree is not defined here, but in ifdata.h, which
// is exported to the device library.  This file is not visible in the
// device library.
//

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
    PT_FUNCTION,
    PT_CONSTANT,
    PT_VAR,
    PT_PARAM,
    PT_TFUNC,
    PT_MACROARG,
    PT_MACRO,
    PT_MACRO_DERIV
};

// These are the functions that we support.
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

// These are the tran functions that we support.
//
enum PTftType
{
    PTF_tNIL,
    PTF_tPULSE,
    PTF_tGPULSE,
    PTF_tPWL,
    PTF_tSIN,
    PTF_tSPULSE,
    PTF_tEXP,
    PTF_tSFFM,
    PTF_tAM,
    PTF_tGAUSS,
    PTF_tINTERP,
    PTF_tTABLE
};

// Cache parameters for tran function evaluation.
//
struct IFtranData
{
    IFtranData()
        {
            td_coeffs = 0;
            td_parms = 0;
            td_cache = 0;
            td_numcoeffs = 0;
            td_type = PTF_tNIL;
            td_pwlRgiven = false;
            td_pwlRstart = 0;
            td_pwlindex = 0;
            td_pwldelay = 0.0;
        }

    IFtranData(PTftType, double*, int, double* = 0, bool = false , int = 0,
        double = 0.0);

    ~IFtranData()
        {
            delete [] td_coeffs;
            delete [] td_parms;
            delete [] td_cache;
        }

    // PULSE
    double V1()      const { return (td_parms[0]); }
    double V2()      const { return (td_parms[1]); }
    double TD()      const { return (td_parms[2]); }
    double TR()      const { return (td_parms[3]); }
    double TF()      const { return (td_parms[4]); }
    double PW()      const { return (td_parms[5]); }
    double PER()     const { return (td_parms[6]); }
    void set_V1(double d)      { td_parms[0] = d; }
    void set_V2(double d)      { td_parms[1] = d; }
    void set_TD(double d)      { td_parms[2] = d; }
    void set_TR(double d)      { td_parms[3] = d; }
    void set_TF(double d)      { td_parms[4] = d; }
    void set_PW(double d)      { td_parms[5] = d; }
    void set_PER(double d)     { td_parms[6] = d; }

    // GPULSE
    double FWHM()    const { return (td_parms[3]); }
    double RPT()     const { return (td_parms[4]); }
    void set_FWHM(double d)    { td_parms[3] = d; }
    void set_RPT(double d)     { td_parms[4] = d; }

    // SIN
    double VO()      const { return (td_parms[0]); }
    double VA()      const { return (td_parms[1]); }
    double FREQ()    const { return (td_parms[2]); }
    double TDL()     const { return (td_parms[3]); }
    double THETA()   const { return (td_parms[4]); }
    double PHI()     const { return (td_parms[5]); }
    void set_VO(double d)      { td_parms[0] = d; }
    void set_VA(double d)      { td_parms[1] = d; }
    void set_FREQ(double d)    { td_parms[2] = d; }
    void set_TDL(double d)     { td_parms[3] = d; }
    void set_THETA(double d)   { td_parms[4] = d; }
    void set_PHI(double d)     { td_parms[5] = d; }

    // SPULSE
    double SPER()    const { return (td_parms[2]); }
    double SDEL()    const { return (td_parms[3]); }
    void set_SPER(double d)    { td_parms[2] = d; }
    void set_SDEL(double d)    { td_parms[3] = d; }

    // EXP
    double TD1()     const { return (td_parms[2]); }
    double TAU1()    const { return (td_parms[3]); }
    double TD2()     const { return (td_parms[4]); }
    double TAU2()    const { return (td_parms[5]); }
    void set_TD1(double d)     { td_parms[2] = d; }
    void set_TAU1(double d)    { td_parms[3] = d; }
    void set_TD2(double d)     { td_parms[4] = d; }
    void set_TAU2(double d)    { td_parms[5] = d; }

    // SFFM
    double FC()      const { return (td_parms[2]); }
    double MDI()     const { return (td_parms[3]); }
    double FS()      const { return (td_parms[4]); }
    void set_FC(double d)      { td_parms[2] = d; }
    void set_MDI(double d)     { td_parms[3] = d; }
    void set_FS(double d)      { td_parms[4] = d; }

    // AM (following Hspice)
    double SA()      const { return (td_parms[0]); }
    double OC()      const { return (td_parms[1]); }
    double MF()      const { return (td_parms[2]); }
    double CF()      const { return (td_parms[3]); }
    double DL()      const { return (td_parms[4]); }
    void set_SA(double d)      { td_parms[0] = d; }
    void set_OC(double d)      { td_parms[1] = d; }
    void set_MF(double d)      { td_parms[2] = d; }
    void set_CF(double d)      { td_parms[3] = d; }
    void set_DL(double d)      { td_parms[4] = d; }

    // GAUSS
    double SD()      const { return (td_parms[0]); }
    double MEAN()    const { return (td_parms[1]); }
    double LATTICE() const { return (td_parms[2]); }
    double ILEVEL()  const { return (td_parms[3]); }
    double LVAL()    const { return (td_parms[4]); }
    double VAL()     const { return (td_parms[5]); }
    double NVAL()    const { return (td_parms[6]); }
    double TIME()    const { return (td_parms[7]); }
    void set_SD(double d)      { td_parms[0] = d; }
    void set_MEAN(double d)    { td_parms[1] = d; }
    void set_LATTICE(double d) { td_parms[2] = d; }
    void set_ILEVEL(double d)  { td_parms[3] = d; }
    void set_LVAL(double d)    { td_parms[4] = d; }
    void set_VAL(double d)     { td_parms[5] = d; }
    void set_NVAL(double d)    { td_parms[6] = d; }
    void set_TIME(double d)    { td_parms[7] = d; }

    void setup(sCKT*, double, double, bool);

    double eval_tPULSE(double);
    double eval_tPULSE_D(double);
    double eval_tGPULSE(double);
    double eval_tGPULSE_D(double);
    double eval_tPWL(double);
    double eval_tPWL_D(double);
    double eval_tSIN(double);
    double eval_tSIN_D(double);
    double eval_tSPULSE(double);
    double eval_tSPULSE_D(double);
    double eval_tEXP(double);
    double eval_tEXP_D(double);
    double eval_tSFFM(double);
    double eval_tSFFM_D(double);
    double eval_tAM(double);
    double eval_tAM_D(double);
    double eval_tGAUSS(double);
    double eval_tGAUSS_D(double);
    double eval_tINTERP(double);
    double eval_tINTERP_D(double);

    IFtranData *dup() const;
    void time_limit(const sCKT*, double*);
    void print(const char*, sLstr&);

private:
    double *td_coeffs;      // tran function parameters (as input)
    double *td_parms;       // tran function run time parameters
    double *td_cache;       // cached stuff for tran evaluation
    int td_numcoeffs;       // number of tran parameters input
    short int td_type;      // function type (PTftType)
    bool td_pwlRgiven;      // true if pwl r=repeat given
    int td_pwlRstart;       // repeat start index, negative for time=0
    int td_pwlindex;        // index into pwl tran func array
    double td_pwldelay;     // pwl td value
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

    IFparseNode *copy(bool = false);
    bool check();
    bool check_macro();
    bool is_const();
    void collapse(IFparseNode**);
    void set_args(const char*);
    double time_limit(sCKT*, double);
    void print_string(sLstr&, TokenType = TT_END, bool = false);
    char *get_string(int = 0);

    static bool parse_if_tranfunc(IFparseNode*, const char*, int*);

    PTtype type()               { return ((PTtype)p_type); }
    IFparseNode *left()         { return (p_left); }
    IFparseNode *right()        { return (p_right); }

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

    void p_init_node(double, double);
    void p_init_func(double, double, bool = false);

    // operations
    double PTplus(const double*);
    double PTminus(const double*);
    double PTtimes(const double*);
    double PTdivide(const double*);
    double PTpower(const double*);

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
    double PTtPulse(const double*);
    double PTtPulseD(const double*);
    double PTtGpulse(const double*);
    double PTtGpulseD(const double*);
    double PTtPwl(const double*);
    double PTtPwlD(const double*);
    double PTtSin(const double*);
    double PTtSinD(const double*);
    double PTtSpulse(const double*);
    double PTtSpulseD(const double*);
    double PTtExp(const double*);
    double PTtExpD(const double*);
    double PTtSffm(const double*);
    double PTtSffmD(const double*);
    double PTtAm(const double*);
    double PTtAmD(const double*);
    double PTtGauss(const double*);
    double PTtGaussD(const double*);
    double PTtInterp(const double*);
    double PTtInterpD(const double*);
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

