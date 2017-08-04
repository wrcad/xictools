
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef SI_PARSER_H
#define SI_PARSER_H

#include "si_if.h"
#include "si_scrfunc.h"


//-----------------------------------------------------------------------------
// Parser Stack Element

// Type of element
enum { ElemNumber, ElemString, ElemNode };
typedef unsigned short PElemType;

// A parser stack element.
struct SIelement
{
    SIelement() { token = TOK_END; type = ElemNumber; value.real = 0.0; }

    PTokenType token;
    PElemType type;
    union {
        char *string;
        double real;
        ParseNode *pnode;
    } value;
};

#define PT_STACKSIZE 200


//-----------------------------------------------------------------------------
// Function and Operator Elements

struct SIptfunc
{
    SIptfunc(const char *n, int ac, unsigned int saflgs, SIscriptFunc f)
        {
            pf_name = n;
            pf_funcptr = f;
            pf_next = 0;
            pf_argc = ac;
            pf_sa_flags = saflgs;
        }

    // Hash table requirements.
    const char *tab_name()          { return (pf_name); }
    SIptfunc *tab_next()            { return (pf_next); }
    SIptfunc *tgen_next(bool)       { return (pf_next); }
    void set_tab_next(SIptfunc *n)  { pf_next = n; }

    SIscriptFunc func()             { return (pf_funcptr); }
    int argc()                      { return (pf_argc); }
    unsigned int string_arg_flags() { return (pf_sa_flags); }

    void set_func(SIscriptFunc f)   { pf_funcptr = f; }
    void set_argc(int ac)           { pf_argc = ac; }

private:
    const char *pf_name;
    SIscriptFunc pf_funcptr;
    SIptfunc *pf_next;
    int pf_argc;
    unsigned int pf_sa_flags;   // mark args to treat as strings
};

struct SIptop
{
    const char *name;
    SIscriptFunc funcptr;
};


template <class T> struct table_t;
struct SIlexp_list;

inline class SIparser *SIparse();

// Main class to implement an expression parser, methods create a parse
// tree from a single expression.
//
class SIparser : public SIif
{
    static SIparser *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline SIparser *SIparse() { return (SIparser::ptr()); }

    SIparser();

    // si_debug.cc
    void printVar(const char*, char*);
    char *setVar(const char*, const char*);
    static char *fromPrintable(const char*);
    static char *toPrintable(const char*);

    // si_lexpr.cc
    char *parseLayer(const char*, char**, char**);
    CDs *openReference(const char*, const char*);

    // si_parser.cc
    void updateInfinity();
    bool registerGlobal(Variable*);
    void registerFunc(const char*, int, SIscriptFunc);
    void unRegisterFunc(const char*);
    void registerAltFunc(const char*, int, unsigned int, SIscriptFunc);
    stringlist *funcList();
    stringlist *altFuncList();
    SIptfunc *function(const char*);
    SIptfunc *altFunction(const char*);
    void clearExprs();
    void pushError(const char*, ...);
    char *popError();
    char *errMessage();
    ParseNode *getTree(const char**, bool);
    ParseNode *getLexprTree(const char**, bool = false);
    bool hasGlobalVariable(const char*);
    bool getGlobalVariable(const char*, siVariable*);
    bool setGlobalVariable(const char*, siVariable*);
    bool evaluate(const char*, char*, int);
    double numberParse(const char**, bool*);

    bool hasError()                     { return (spErrMsgs != 0); }

    void setVariables(siVariable *v)    { spVariables = v; }
    siVariable *getVariables()          { return (spVariables); }
    void presetVariables(siVariable *v) { spVariablesPreset = v; }

    siVariable *findVariable(const char *name)
        {
            if (!name || !*name)
                return (0);
            for (siVariable *v = spVariables; v; v = (siVariable*)v->next) {
                if (v->name && !strcmp(name, v->name))
                    return (v);
            }
            return (0);
        }

    void addVariable(siVariable *v)
        // Build from the end so we can set up external variables at
        // the beginning of the list and find them there after the
        // tree is built and evaluated.
        {
            siVariable *vv = spVariables;
            if (!vv)
                spVariables = v;
            else {
                while (vv->next)
                    vv = (siVariable*)vv->next;
                vv->next = v;
            }
        }

    void clearVariables()
        {
            siVariable::destroy(spVariables);
            spVariables = spVariablesPreset;
            spVariablesPreset = 0;
        }

    void addGlobal(siVariable *v)
        { v->next = spGlobals; spGlobals = v; }
    void clearGlobals()
        { siVariable::destroy(spGlobals); spGlobals = 0; }

    siVariable *findGlobal(const char *name)
        {
            if (!name || !*name)
                return (0);
            for (siVariable *v = spGlobals; v; v = (siVariable*)v->next) {
                if (v->name && !strcmp(name, v->name))
                    return (v);
            }
            return (0);
        }

    bool setUseAltFuncs(bool b)
        {
            bool t = spUseAltFuncs;
            spUseAltFuncs = b;
            return (t);
        }

    void setSuppressErrInitFunc(SIscriptFunc f)
        { spSuppressErrInitFunc = f; }
    SIscriptFunc getSuppressErrInitFunc()
        { return (spSuppressErrInitFunc); }

    SIlexp_list *setExprs(SIlexp_list *ex)
        {
            SIlexp_list *oexp = spExprs;
            spExprs = ex;
            return (oexp);
        }

    // Sometime we need to hook sub-trees together.
    ParseNode *MakeUopNode(PTokenType t, ParseNode *p)
        { return (mkunode(t, p)); }
    ParseNode *MakeBopNode(PTokenType t, ParseNode *pl, ParseNode *pr)
        { return (mkbnode(t, pl, pr)); }

    bool isSubFunc(const ParseNode *p)
        { return (p->type == PT_FUNCTION && p->evfunc == pt_switch); }

private:
    // funcs_lexpr.cc
    void funcs_lexpr_init();

    // funcs_math.cc
    void funcs_math_init();

    // si_parser.cc
    ParseNode *parse(const char**);
    SIelement *lexer(const char**);
    char *grab_string(const char**);
    ParseNode *makepnode(SIelement*);
    ParseNode *mkbnode(PTokenType, ParseNode*, ParseNode*);
    ParseNode *mkunode(PTokenType, ParseNode*);
    ParseNode *mkfnode(char*, ParseNode*);
    ParseNode *mknnode(double);
    ParseNode *mksnode(char*, ParseNode*);
    siVariable *mkvar(char*, int*);

    // The evaluation functions.
    static bool pt_const(ParseNode*, siVariable*, void*);
    static bool pt_var(ParseNode*, siVariable*, void*);
    static bool pt_fcn(ParseNode*, siVariable*, void*);
    static bool pt_assign(ParseNode*, siVariable*, void*);
    static bool pt_switch(ParseNode*, siVariable*, void*);
    static bool pt_bop(ParseNode*, siVariable*, void*);
    static bool pt_cond(ParseNode*, siVariable*, void*);
    static bool pt_uop(ParseNode*, siVariable*, void*);

    stringlist *spErrMsgs;          // list of detail error messages
    siVariable *spGlobals;          // variables with global scope
    siVariable *spVariables;        // current variables
    siVariable *spVariablesPreset;  // moved to spVariables when Clear called
    siVariable *spLexpVars;         // layer expression variables
       // The layer expression variables correspond to layer names.
       // Since layer expressions never call one another, we can keep a
       // single list of variables used in layer expressions.

    SymTab *spConstTab;             // hash table for named constants
    table_t<SIptfunc> *spIFfuncs;   // interface function table
    table_t<SIptfunc> *spALTfuncs;  // installable function table
    // Hack:  address of "AddError" script function, used to suppress
    // initialization of the error handler so we can add error
    // message strings.
    SIscriptFunc spSuppressErrInitFunc;

    SIlexp_list *spExprs;           // layer expression list

    int spTriNest;                  // lexer context
    int spLastToken;                // lexer context
    PElemType spLastType;           // lexer context
    bool spUseAltFuncs;             // use alternate function table
    bool spGlobalOnly;              // use only global variables in parse

    static SIptop spPTops[];        // operator table (funcs_math.cc)
    static PTconstant spConstants[];  // list of predefined constants
    static SIparser *instancePtr;
};

#endif

