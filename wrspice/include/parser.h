
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Wayne A. Christopher, U. C. Berkeley CAD Group
**********/

#ifndef PARSER_H
#define PARSER_H

typedef pnode ParseNode;
#include "datavec.h"
#include "spnumber/spparse.h"


//
// Stuff for parsing -- used by the parser and in SP.Eevaluate().
//

typedef sDataVec*(sDataVec::*opFuncType)(sDataVec*);

// Operations. These should really be considered functions.
//
struct sOper
{
    sOper(TokenType n, const char *na, int ac, opFuncType f)
        {
            op_num = n;
            op_name = na;
            op_arity = ac;
            op_func = f;
        }

    TokenType optype()      { return (op_num); }
    const char *name()      { return (op_name); }
    int argc()              { return (op_arity); }
    opFuncType func()       { return (op_func); }

private:
    TokenType op_num;       // From parser #defines.
    const char *op_name;    // Printing name.
    char op_arity;          // One or two.
    opFuncType op_func;     // Evaluation method.
};


typedef sDataVec*(sDataVec::*fuFuncType)();

// The functions that are available.
//
struct sFunc
{
    sFunc(const char *na, fuFuncType f, int ac)
        {
            fu_name = na;
            fu_func = f;
            fu_nargs = ac;
        }

    const char *name()      { return (fu_name); }
    void set_name(const char *n) { fu_name = n; }
    fuFuncType func()       { return (fu_func); }
    int argc()              { return (fu_nargs); }

private:
    const char *fu_name;    // The print name of the function;
    fuFuncType fu_func;     // the sDataVec evaluation method;
    int fu_nargs;           // Argument count;
};


// User-definable functions. The idea of ud_name is that the args are
// kept in strings after the name, all seperated by '\0's. There
// will be ud_arity of them.
//
struct sUdFunc
{
    sUdFunc(const char *n, int a, pnode *p)
        {
            ud_name = n;
            ud_arity = a;
            ud_text = p;
            ud_next = 0;
        }

    ~sUdFunc();

    static sUdFunc *copy(const sUdFunc*);
    void reuse(char*, pnode*);

    const char *name()          { return (ud_name); }
    int argc()                  { return (ud_arity); }
    pnode *tree()               { return (ud_text); }
    sUdFunc *next()             { return (ud_next); }
    void set_next(sUdFunc *n)   { ud_next = n; }

private:
    const char *ud_name;    // The name
    int ud_arity;           // The arity of the function
    pnode *ud_text;         // The definition
    sUdFunc *ud_next;       // Link pointer
};


// Manage a collection of user-defined functions.
//
class cUdf
{
public:
    cUdf() { ud_tab = 0; ud_promo_tab = 0; }
    ~cUdf();

    bool parse(wordlist*, char**, char**) const;
    bool define(wordlist*);
    bool define(const char*, const char*);
    bool define_unique(const char**, const char*, const char*);
    bool is_defined(const char*, int) const;
    bool get_macro(const char*, int, char**, const char**) const;
    sUdFunc *find(const char*, int) const;
    pnode *get_tree(sUdFunc*, const pnode*) const;
    void clear(wordlist*);
    cUdf *copy() const;
    void print() const;

    sHtab *table() { return (ud_tab); }

private:
    void new_unique_name(const char**);
    const char *get_promoted(const char*, const char*);
    void set_promoted(const char*, const char*, const char*);

    sHtab *ud_tab;
    sHtab *ud_promo_tab;
};

#define MAXARITY 32


// Parse node data struct.
//

struct sLstr;
struct sParamTab;
struct sMacroMapTab;

// pnode type field
enum PNtype { PN_NONE, PN_VEC, PN_TRAN };

struct pnode
{
    pnode(sOper *op, pnode *lpn, pnode *rpn)
        {
            pn_name = 0;
            pn_string = 0;
            pn_value = 0;
            pn_func = 0;
            pn_op = op;
            pn_left = lpn;
            pn_right = rpn;
            pn_type = PN_NONE;
            pn_localval = false;
        }

    pnode(sFunc *fu, pnode *lpn, bool local = false)
        {
            pn_name = 0;
            pn_string = 0;
            pn_value = 0;
            pn_func = fu;
            pn_op = 0;
            pn_left = lpn;
            pn_right = 0;
            pn_type = PN_NONE;
            pn_localval = local;
        }

    pnode(const char *na, const char *str, sDataVec *val, bool local = false)
        {
            pn_name = lstring::copy(na);
            pn_string = lstring::copy(str);
            pn_value = val;
            pn_func = 0;
            pn_op = 0;
            pn_left = 0;
            pn_right = 0;
            pn_type = PN_NONE;
            pn_localval = local;
        }

    // parse.cc
    ~pnode();

    bool checkvalid()               const;
    int checktree()                 const;
    void collapse(pnode**);
    pnode *expand_macros();
    void promote_macros(const sParamTab*, sMacroMapTab* = 0);
    pnode *vecstring(const char*);
    void get_string(sLstr&, TokenType = TT_END, bool = false) const;
    char *get_string(bool = false)  const;
    pnode *copy(const char* = 0, const pnode* = 0) const;
    void copyvecs();

    // evaluate.cc
    sDataVec *apply_func()          const;
    sDataVec *apply_bop()           const;

    const char *name()              const { return (pn_name); }
    void set_name(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] pn_name;
            pn_name = s;
        }

    TokenType optype()              const
        {
            if (pn_op)
                return (pn_op->optype());
            return (TT_END);
        }

    bool is_const()
        {
            return (pn_string && pn_value && pn_type == PN_NONE);
        }

    bool is_const_one()
        {
            return (is_const() && pn_value->isreal() &&
                pn_value->length() == 1 && pn_value->realval(0) == 1.0);
        }

    bool is_const_zero()
        {
            return (is_const() && pn_value->isreal() &&
                pn_value->length() == 1 && pn_value->realval(0) == 0.0);
        }

    const char *token_string()      const { return (pn_string); }
    sDataVec *value()               const { return (pn_value); }
    sFunc *func()                   const { return (pn_func); }
    sOper *oper()                   const { return (pn_op); }
    pnode *left()                   const { return (pn_left); }
    pnode *right()                  const { return (pn_right); }
    PNtype type()                   const { return (pn_type); }
    void set_type(PNtype t)         { pn_type = t; }
    bool is_localval()              const { return (pn_localval); }

private:
    char *pn_name;          // If non-0, the name.
    char *pn_string;        // Non-0 in a terminal node.
    sDataVec *pn_value;     // Non-0 in a constant node.
    sFunc *pn_func;         // Non-0 is a function.
    sOper *pn_op;           // Operation if the above 0.
    pnode *pn_left;         // Left branch or function argument.
    pnode *pn_right;        // Right branch.
    PNtype pn_type;         // Data type.
    bool pn_localval;       // The pn_value is local, free with object.
};

// List of pnodes.
struct pnlist
{
    pnlist(pnode *e, pnlist *n)
        {
            pnl_next = n;
            pnl_node = e;
        }

    ~pnlist()
        {
            delete pnl_node;
        }

    static void destroy(pnlist *pl)
        {
            while (pl) {
                pnlist *px = pl;
                pl = pl->next();
                delete px;
            }
        }

    pnlist *next()              { return (pnl_next); }
    void set_next(pnlist *n)    { pnl_next = n; }

    pnode *node()               { return (pnl_node); }

private:
    pnlist *pnl_next;
    pnode *pnl_node;
};

#endif // FTEPARSE_H

