
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
 $Id: si_lisp.h,v 5.15 2017/03/09 01:12:04 stevew Exp $
 *========================================================================*/

#ifndef SI_LISP_H
#define SI_LISP_H

//
// Definitions for the LISP parser
//

// lispnode types
//
// LN_NODE      function or list
// LN_STRING    unquoted string
// LN_NUMERIC   number
// These are returned from the parser, but never by functions, operators,
// or evaluators:
// LN_QSTRING   quoted string
// LN_OPER      operator
//
enum LN_TYPE { LN_NODE, LN_STRING, LN_NUMERIC, LN_OPER, LN_QSTRING };

class cLispEnv;
struct Variable;

// lispnode::  [name]( lispnode ... )
//
// A lispnode is a representation of a list.  If the opening paren is
// prefixed with a name with *no space*, the lispnode will be "named".
// e.g.
//  alist( 1 2 3 )      a named list
//  alist ( 1 2 3 )     two parser tokens
//
// a lispnode is one of the following types:
//   string             contains a string token
//   numeric            contains a numeric value
//   assign             contains a name = (lispnode)value description
//   node               contains a named or unnamed list of lispnodes
//
struct lispnode
{
    lispnode();
    lispnode(LN_TYPE, char*);

    static lispnode *dup(const lispnode*);
    static lispnode *dup_list(const lispnode*);
    static void destroy(const lispnode*);
    static bool print(const lispnode*, FILE*, int, bool);
    static void print(const lispnode*, sLstr*);

    static int eval_list(lispnode*, lispnode*, int, char**);
    static bool eval_list(lispnode*, lispnode*, char**);
    bool eval(lispnode*, char**);
    int eval_xic_node(lispnode*, char**);
    int eval_user_node(lispnode*, char**);
    bool ln_to_var(Variable*, char**);
    bool var_to_ln(Variable*, char**);

    int arg_cnt() const
        {
            int cnt = 0;
            for (lispnode *n = args; n; n = n->next, cnt++) ;
            return (cnt);
        }

    bool is_nil() const
        {
            return (type == LN_NODE && !string && !args);
        }

    void set_nil()
        {
            type = LN_NODE;
            string = 0; args = 0;
        }

    void set(lispnode *n)
        {
            if (!n)
                set_nil();
            else {
                *this = *n; next = 0;
            }
        }

    static cLispEnv *get_env()
        {
            return (lisp_env);
        }

    static cLispEnv *set_env(cLispEnv *e)
        {
            cLispEnv *old = lisp_env;
            lisp_env = e;
            return (old);
        }

    LN_TYPE type;       // type of node
    char *string;       // string token
    lispnode *args;     // arguments for LN_NODE, rhs for LN_OPER
    lispnode *lhs;      // lhs for LN_OPER
    lispnode *next;
    double value;       // numeric value for LN_NUMERIC

private:
    static cLispEnv *lisp_env;
};

// function type for dispatch handlers
typedef bool(*nodefunc)(lispnode*, lispnode*, char**);


// Main for Lisp execution environment.
//
class cLispEnv
{
public:
    // List of blocks of lispnodes for temp allocator.
    struct lnlist { lispnode *nodes; lnlist *next; };

    cLispEnv();
    ~cLispEnv();

    bool readEvalLisp(const char*, const char*, bool, char**);

    void register_node(const char*, nodefunc);
    void register_user_node(lispnode*);
    nodefunc find_func(const char*);
    lispnode *find_user_node(const char*);
    lispnode *new_temp_node();
    lispnode *new_temp_copy(lispnode*);
    bool set_variable(const char*, lispnode*, char**);
    bool get_variable(lispnode*, const char*);
    void push_local_vars(lispnode*p);
    void pop_local_vars();

    static bool is_logging()        { return (le_logging); }
    static void set_logging(bool b) { le_logging = b; }

    static bool get_lisp_node(const char**, lispnode**, lispnode**);
    static lispnode *get_lisp_list(const char **s)
        {
            lispnode *ph = 0, *pt = 0;
            while (get_lisp_node(s, &ph, &pt)) ;
            return (ph);
        }

private:
    lispnode *parseLisp(const char*, char**);
    lispnode *find_local_var(const char*);
    bool test_variable(const char*);
    void recycle(lispnode*);
    void clear();

    SymTab *le_nodetab;
    SymTab *le_global_vartab;
    SymTab *le_user_nodes;
    lispnode *le_local_vars;
    lnlist *le_tln_blocks;
    int le_tln_index;
    lispnode *le_recycle_list;

    static bool le_logging;
};

#endif

