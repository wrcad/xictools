
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

#include "cd.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lisp.h"
#include "spnumber.h"
#include "pathlist.h"
#include "random.h"
#include "errorlog.h"
#include "filestat.h"

#include <ctype.h>
#include <time.h>
#include <math.h>


//
// Parser for Lisp-like rule sets
//

namespace {
    // operators
    bool expt(lispnode*, lispnode*, char**);
    bool times(lispnode*, lispnode*, char**);
    bool quotient(lispnode*, lispnode*, char**);
    bool plus(lispnode*, lispnode*, char**);
    bool difference(lispnode*, lispnode*, char**);
    bool lessp(lispnode*, lispnode*, char**);
    bool leqp(lispnode*, lispnode*, char**);
    bool greaterp(lispnode*, lispnode*, char**);
    bool geqp(lispnode*, lispnode*, char**);
    bool equal(lispnode*, lispnode*, char**);
    bool nequal(lispnode*, lispnode*, char**);
    bool and_node(lispnode*, lispnode*, char**);
    bool or_node(lispnode*, lispnode*, char**);
    bool colon(lispnode*, lispnode*, char**);
    bool setq(lispnode*, lispnode*, char**);

    // math
    bool abs(lispnode*, lispnode*, char**);
    bool sgn(lispnode*, lispnode*, char**);
    bool acos(lispnode*, lispnode*, char**);
    bool asin(lispnode*, lispnode*, char**);
    bool atan(lispnode*, lispnode*, char**);
#ifdef HAVE_ACOSH
    bool acosh(lispnode*, lispnode*, char**);
    bool asinh(lispnode*, lispnode*, char**);
    bool atanh(lispnode*, lispnode*, char**);
#endif // HAVE_ACOSH
    bool cos(lispnode*, lispnode*, char**);
    bool cosh(lispnode*, lispnode*, char**);
    bool exp(lispnode*, lispnode*, char**);
    bool ln(lispnode*, lispnode*, char**);
    bool log(lispnode*, lispnode*, char**);
    bool log10(lispnode*, lispnode*, char**);
    bool sin(lispnode*, lispnode*, char**);
    bool sinh(lispnode*, lispnode*, char**);
    bool sqrt(lispnode*, lispnode*, char**);
    bool tan(lispnode*, lispnode*, char**);
    bool tanh(lispnode*, lispnode*, char**);
    bool atan2(lispnode*, lispnode*, char**);
    bool seed(lispnode*, lispnode*, char**);
    bool random(lispnode*, lispnode*, char**);
    bool gauss(lispnode*, lispnode*, char**);
    bool floor(lispnode*, lispnode*, char**);
    bool ceil(lispnode*, lispnode*, char**);
    bool rint(lispnode*, lispnode*, char**);
    bool tint(lispnode*, lispnode*, char**);
    bool max(lispnode*, lispnode*, char**);
    bool min(lispnode*, lispnode*, char**);

    // flow control
    bool main_node(lispnode*, lispnode*, char**);
    bool procedure(lispnode*, lispnode*, char**);
    bool let(lispnode*, lispnode*, char**);
    bool if_node(lispnode*, lispnode*, char**);
    bool when_node(lispnode*, lispnode*, char**);
    bool unless_node(lispnode*, lispnode*, char**);
    bool case_node(lispnode*, lispnode*, char**);
    bool for_node(lispnode*, lispnode*, char**);
    bool foreach_node(lispnode*, lispnode*, char**);

    // lists
    bool squote_node(lispnode*, lispnode*, char**);
    bool list(lispnode*, lispnode*, char**);
    bool cons(lispnode*, lispnode*, char**);
    bool append(lispnode*, lispnode*, char**);
    bool car(lispnode*, lispnode*, char**);
    bool cdr(lispnode*, lispnode*, char**);
    bool nth(lispnode*, lispnode*, char**);
    bool member(lispnode*, lispnode*, char**);
    bool length(lispnode*, lispnode*, char**);
    bool xCoord(lispnode*, lispnode*, char**);
    bool yCoord(lispnode*, lispnode*, char**);

    // Operators
    struct sLoper
    {
        const char *name;
        const char *string;
        int precedence;
    };

    // The recognized operators, order is important: "**" before "*", etc
    //
    sLoper Lopers[] =
    {
        { "expt",       "**", 2  },
        { "times",      "*",  7  },
        { "quotient",   "/",  7  },
        { "plus",       "+",  5  },
        { "difference", "-",  5  },
        { "leqp",       "<=", 3  },
        { "lessp",      "<",  3  },
        { "geqp",       ">=", 3  },
        { "greaterp",   ">",  3  },
        { "equal",      "==", 3  },
        { "nequal",     "!=", 3  },
        { "and",        "&&", 6  },
        { "or",         "||", 4  },
        { "setq",       "=",  0  },
        { "colon",      ":",  1  },
        { 0,            0,    0  }
    };


    // Formulate an error message.
    //
    void errset(char **err, const char *msg, const char *word)
    {
        if (!msg)
            msg = "%s";
        if (*err) {
            char *t = *err;
            if (word) {
                *err = new char[strlen(msg) + strlen(word) + strlen(t) + 1];
                sprintf(*err, msg, word);
                strcat(*err, "\n");
                strcat(*err, t);
                delete [] t;
            }
            else {
                *err = new char[strlen(msg) + strlen(t)];
                sprintf(*err, msg, t);
                delete [] t;
            }
        }
        else {
            if (!word)
                word = "unknown error";
            *err = new char[strlen(msg) + strlen(word)];
            sprintf(*err, msg, word);
        }
    }
}


//-----------------------------------------------------------------------
// The LispEnv
//-----------------------------------------------------------------------

// Memory Management
// The parser returns a lispnode tree where all data objects are
// allocated from the system.  This should be destroyed with
// lispnode::destroy();
//
// User-defined nodes in the user_nodes table should be destroyed
// using lispnode::destroy()
//
// During evaluation, all new lisp nodes that are not on the local
// stack are obtained with new_temp_node().  These can be given back
// to the allocator with recycle().  They should never be freed or
// deleted.

bool cLispEnv::le_logging;

cLispEnv::cLispEnv()
{
    le_nodetab = 0;
    le_global_vartab = 0;
    le_user_nodes = 0;
    le_local_vars = 0;
    le_tln_blocks = 0;
    le_tln_index = 0;
    le_recycle_list = 0;

    // operators
    register_node("expt",       expt);
    register_node("times",      times);
    register_node("quotient",   quotient);
    register_node("plus",       plus);
    register_node("difference", difference);
    register_node("lessp",      lessp);
    register_node("leqp",       leqp);
    register_node("greaterp",   greaterp);
    register_node("geqp",       geqp);
    register_node("equal",      equal);
    register_node("nequal",     nequal);
    register_node("and",        and_node);
    register_node("or",         or_node);
    register_node("colon",      colon);
    register_node("setq",       setq);

    // math
    register_node("abs", abs);
    register_node("sgn", sgn);
    register_node("acos", acos);
    register_node("asin", asin);
    register_node("atan", atan);
#ifdef HAVE_ACOSH
    register_node("acosh", acosh);
    register_node("asinh", asinh);
    register_node("atanh", atanh);
#endif // HAVE_ACOSH
    register_node("cos", cos);
    register_node("cosh", cosh);
    register_node("exp", exp);
    register_node("ln", ln);
    register_node("log", log);
    register_node("log10", log10);
    register_node("sin", sin);
    register_node("sinh", sinh);
    register_node("sqrt", sqrt);
    register_node("tan", tan);
    register_node("tanh", tanh);
    register_node("atan2", atan2);
    register_node("seed", seed);
    register_node("random", random);
    register_node("gauss", gauss);
    register_node("floor", floor);
    register_node("ceil", ceil);
    register_node("rint", rint);
    register_node("int", tint);
    register_node("max", max);
    register_node("min", min);

    // flow control
    register_node("main",       main_node);
    register_node("procedure",  procedure);
    register_node("let",        let);
    register_node("if",         if_node);
    register_node("when",       when_node);
    register_node("unless",     unless_node);
    register_node("case",       case_node);
    register_node("for",        for_node);
    register_node("foreach",    foreach_node);

    // lists
    register_node("'",          squote_node);
    register_node("list",       list);
    register_node("cons",       cons);
    register_node("append",     append);
    register_node("car",        car);
    register_node("cdr",        cdr);
    register_node("nth",        nth);
    register_node("member",     member);
    register_node("length",     length);
    register_node("xCoord",     xCoord);
    register_node("yCoord",     yCoord);
}


cLispEnv::~cLispEnv()
{
    delete le_nodetab;
    if (le_user_nodes) {
        SymTabGen st(le_user_nodes, true);
        SymTabEnt *ent;
        while ((ent = st.next()) != 0) {
            lispnode::destroy((lispnode*)ent->stData);
            delete ent;
        }
    }
    delete le_user_nodes;
    clear();
}


// The first argument is a string giving a file name and optionally an
// argument list.  The second argument is a search path for the file.
// If cwdfirst, the cwd is checked before the path.  The file is
// opened and parsed as lisp, and the tree executed.
//
bool
cLispEnv::readEvalLisp(const char *s, const char *search_path, bool cwdfirst,
    char **err)
{
    *err = 0;
    const char *cline = s;
    char *fname = lstring::gettok(&s);
    if (!fname) {
        *err = lstring::copy("no filename given");
        return (false);
    }
    char *rpath;
    FILE *fp = pathlist::open_path_file(fname, search_path, "r", &rpath,
        cwdfirst);
    if (!fp) {
        errset(err, "can't open %s", fname);
        delete [] fname;
        return (false);
    }
    fclose(fp);

    lispnode *p0 = parseLisp(rpath, err);
    delete [] fname;
    delete [] rpath;
    if (!p0) {
        errset(err, "parse failed, %s", 0);
        return (false);
    }

    lispnode *av = get_lisp_list(&cline);
    lispnode tmpav;
    tmpav.args = av;
    int ac = tmpav.arg_cnt();
    lispnode tmpac;
    tmpac.type = LN_NUMERIC;
    tmpac.value = ac;
    set_variable("argc", &tmpac, err);
    set_variable("argv", &tmpav, err);

    SI()->PushLexprCx();
    cLispEnv *etmp = lispnode::set_env(this);
    bool retval = true;
    lispnode res;
    for (lispnode *p = p0; p; p = p->next) {
        if (!p->eval(&res, err)) {
            errset(err, "execution error, %s", 0);
            retval = false;
            break;
        }
    }
    lispnode::destroy(p0);
    clear();
    lispnode::set_env(etmp);
    SI()->PopLexprCx();
    return (retval);
}


// Add a dispatch function to the symbol table.
//
void
cLispEnv::register_node(const char *name, nodefunc func)
{
    if (!le_nodetab)
        le_nodetab = new SymTab(true, false);
    if (!le_nodetab->add(lstring::copy(name), (void*)func, true))
        fprintf(stderr, "cLispEnv warning: duplicate node %s", name);
}


void
cLispEnv::register_user_node(lispnode *p)
{
    if (!le_user_nodes)
        le_user_nodes = new SymTab(false, false);
    lispnode *px = (lispnode*)SymTab::get(le_user_nodes, p->string);
    if (px && px != (lispnode*)ST_NIL) {
        le_user_nodes->remove(p->string);
        lispnode::destroy(px);
    }
    le_user_nodes->add(p->string, p, false);
}


// Return a dispatch function given the name.
//
nodefunc
cLispEnv::find_func(const char *name)
{
    if (!le_nodetab)
        return (0);
    nodefunc func = (nodefunc)SymTab::get(le_nodetab, name);
    if (func == (nodefunc)ST_NIL)
        return (0);
    return (func);
}


lispnode *
cLispEnv::find_user_node(const char *name)
{
    if (!le_user_nodes)
        return (0);
    lispnode *p = (lispnode*)SymTab::get(le_user_nodes, name);
    if (p == (lispnode*)ST_NIL)
        return (0);
    return (p);
}


#define TLN_BL_SIZE 128

// Return a new lispnode for use in building lists.  These should
// never be freed.
//
lispnode*
cLispEnv::new_temp_node()
{
    if (le_recycle_list) {
        lispnode *p = le_recycle_list;
        le_recycle_list = le_recycle_list->next;
        p->next = 0;
        return (p);
    }
    if (!le_tln_blocks || le_tln_index == TLN_BL_SIZE) {
        lnlist *l = new lnlist;
        l->nodes = new lispnode[TLN_BL_SIZE];
        l->next = le_tln_blocks;
        le_tln_blocks = l;
        le_tln_index = 0;
    }
    return (&le_tln_blocks->nodes[le_tln_index++]);
}


// Return a copy of the lispnode passed, should never be freed.
//
lispnode *
cLispEnv::new_temp_copy(lispnode *p0)
{
    lispnode *l0 = 0, *le = 0;
    while (p0) {
        lispnode *l = new_temp_node();
        l->set(p0);
        if (!l0)
            l0 = l;
        else
            le->next = l;
        le = l;
    }
    return (l0);
}


// Set a variable, add to table if not already there.
//
bool
cLispEnv::set_variable(const char *string, lispnode *vrhs, char**)
{
    if (!le_global_vartab)
        le_global_vartab = new SymTab(true, false);
    lispnode *v = (lispnode*)SymTab::get(le_global_vartab, string);
    if (v == (lispnode*)ST_NIL) {
        v = new_temp_node();
        le_global_vartab->add(lstring::copy(string), v, false);
    }
    v->set(vrhs);
    return (true);
}


// Obtain a variable by name, copied into res.
//
bool
cLispEnv::get_variable(lispnode *ret, const char *name)
{
    lispnode *v;
    if (!le_global_vartab ||
            (v = (lispnode*)SymTab::get(le_global_vartab, name)) ==
            (lispnode*)ST_NIL) {
        v = find_local_var(name);
        if (!v)
            return (false);
    }
    ret->set(v);
    return (true);
}


//
// Local variables
//
// The local variables are saved in a stack implemented with
// lispnodes, each level is a list of lispnodes where the string is
// the variable name and the args field is the actual variable.
//

void
cLispEnv::push_local_vars(lispnode *p)
{
    lispnode *dmy = new_temp_node();
    dmy->args = p;
    dmy->next = le_local_vars;
    le_local_vars = dmy;
}


void
cLispEnv::pop_local_vars()
{
    lispnode *dmy = le_local_vars;
    if (dmy) {
        le_local_vars = le_local_vars->next;
        lispnode *an;
        for (lispnode *a = dmy->args; a; a = an) {
            an = a->next;
            recycle(a->args);
            recycle(a);
        }
        recycle(dmy);
    }
}


namespace {
    const char *termchars = "()=<>!*+-/";

    bool isterm(char c)
    {
        return (strchr(termchars, c) != 0);
    }


    sLoper *is_oper(const char **pstr)
    {
        const char *str = *pstr;
        while (isspace(*str))
            str++;
        for (sLoper *op = Lopers; op->name; op++) {
            const char *s1 = str;
            const char *s2 = op->string;
            if (*s1++ == *s2++) {
                if (!*s2 || *s1 == *s2) {
                    // Assume a unary operator if it is preceded by space
                    // or '(' and is followed immediately by an integer or
                    // period followed by an integer.

                    if ((*str == '-' || *str == '+') &&
                            (isdigit(*(str+1)) ||
                                (*(str+1) == '.' && isdigit(*(str+2)))) &&
                            (isspace(*(str-1)) || (*(str-1) == '(')))
                        return (0);

                    *pstr = str + strlen(op->string);
                    return (op);
                }
            }
        }
        return (0);
    }


    // Terminate on '(', ')', and operators, return double quoted tokens.
    //
    char *get_lisp_tok(const char **s, sLoper **isop = 0)
    {
        if (isop)
            *isop = 0;
        if (s == 0)
            return (0);
        const char *t = *s;
        while (isspace(*t))
            t++;
        if (!*t) {
            *s = t;
            return (0);
        }

        const char *st = t;
        if (*t == '(' || *t == ')')
            t++;
        else if (*t == '"') {
            t++;
            while (*t && (*t != '"' || *(t-1) == '\\'))
                t++;
            if (*t == '"')
                t++;
        }
        else {
            sLoper *op = is_oper(&t);
            if (!op) {
                if (*t == '+')
                    t++;
                const char *tt = t;
                if (SPnum.parse(&tt, false)) {
                    while (*t && !isspace(*t) && t != tt)
                        t++;
                }
                else {
                    char c = 0;
                    while (*t && !isspace(*t)) {
                        if (c != '\\' && isterm(*t))
                            break;
                        c = *t++;
                    }
                }
            }
            else if (isop)
                *isop = op;
        }
        char *cbuf = new char[t - st + 1];
        char *c = cbuf;
        while (st < t)
            *c++ = *st++;
        *c = 0;
        *s = t;
        return (cbuf);
    }


    // Pull the last node off the list.
    //
    lispnode *unlink(lispnode **head, lispnode **tail)
    {
        if (*tail) {
            lispnode *pt = *tail;
            lispnode *ph = *head;
            if (!ph->next) {
                *head = 0;
                *tail = 0;
                return (pt);
            }
            while (ph->next != *tail)
                ph = ph->next;
            ph->next = 0;
            *tail = ph;
            return (pt);
        }
        return (0);
    }
}


// Static function.
// Parse s as far as possible, returning a tree of lispnodes.
//
bool
cLispEnv::get_lisp_node(const char **s, lispnode **head, lispnode **tail)
{
    sLoper *op;
    char *tok = get_lisp_tok(s, &op);
    if (!tok)
        return (false);
    const char *ctok = tok;

    double *dp;
    lispnode *p = 0;
    if (*tok == '"') {
        // "string"
        // strip quotes
        char *stmp = tok + strlen(tok) - 1;
        if (*stmp == '"' && stmp != tok)
            *stmp = 0;
        for (stmp = tok; *stmp; stmp++)
            *stmp = *(stmp+1);
        p = new lispnode(LN_QSTRING, tok);
    }
    else if (*tok == '(') {
        // ( ... )
        p = new lispnode(LN_NODE, 0);
        delete [] tok;
        p->args = get_lisp_list(s);
    }
    else if (*tok == ')') {
        delete [] tok;
        return (false);
    }
    else if (op) {
        delete [] tok;
        p = new lispnode(LN_OPER, (char*)op->string);
        lispnode *pl = unlink(head, tail);
        lispnode *pt = 0;
        get_lisp_node(s, &p->args, &pt);
        sLoper *nop;
        while ((nop = is_oper(s)) != 0) {
            *s -= strlen(nop->string);
            if (op->precedence <= nop->precedence) {
                // eg, a + b * c
                // --> +(a, *(b, c))
                get_lisp_node(s, &p->args, &pt);
                continue;
            }
            else {
                // eg, a * b + c
                // --> +(*(a,b), c)

                // p = *(nil, b)
                get_lisp_node(s, &p->args, &pt);
                // p = *(nil, +(b, c))

                lispnode *t = p;
                p = p->args; // p = +(b, c), t = *(nil, +(b,c))
                t->args = t->args->lhs;   // t = *(nil, b)
                t->lhs = pl;              // t = *(a, b)
                pl = t;
                p->lhs = 0;
                op = nop;

                pt = p->args;
                if (pt) {
                    while (pt->next)
                        pt = pt->next;
                }
            }
        }
        p->lhs = pl;
    }
    else if ((dp = SPnum.parse(&ctok, true)) != 0) {
        // number
        p = new lispnode;
        p->type = LN_NUMERIC;
        p->string = tok;
        p->value = *dp;
    }
    else if (**s == '(') {
        // name( ... )
        (*s)++;
        p = new lispnode(LN_NODE, tok);
        p->args = get_lisp_list(s);
    }
    else
        p = new lispnode(LN_STRING, tok);

    if (*tail)
        (*tail)->next = p;
    else
        (*head) = p;
    *tail = p;
    return (true);
}


// private members

namespace {
#define CMT_START "/""*"
#define CMT_END "*""/"

    // Remove C-style comments and trailing white space.
    //
    char *uncmt(char *txt, bool *in_cmt)
    {
        if (*in_cmt) {
            char *t = strstr(txt, CMT_END);
            if (t) {
                txt = t + strlen(CMT_END);
                *in_cmt = false;
            }
            else
                return (0);
        }
        char buf[1024];
        buf[0] = 0;
        for (;;) {
            char *t = strstr(txt, CMT_START);
            if (t) {
                *in_cmt = true;
                char *s = buf + strlen(buf);
                strncpy(s, txt, t - txt);
                s[t - txt] = 0;
                t += strlen(CMT_START);
                char *e = strstr(t, CMT_END);
                if (e) {
                    *in_cmt = false;
                    txt = e + strlen(CMT_END);
                    continue;
                }
                break;
            }
            else {
                strcat(buf, txt);
                break;
            }
        }

        // ';' comments
        bool inq = false;
        for (char *t = buf; *t; t++) {
            if (*t == '"')
                inq = !inq;
            else if (*t == ';' && !inq) {
                *t = 0;
                break;
            }
        }

        char *t = buf + strlen(buf) - 1;
        while (t >= buf && isspace(*t))
            *t-- = 0;
        t = buf;
        while (isspace(*t))
            t++;
        if (*t)
            return (lstring::copy(t));
        return (0);
    }


    // Read the file, return a list of precessed lines.
    //
    stringlist *slurp_file(FILE *fp)
    {
        stringlist *s0 = 0, *se = 0;
        bool in_cmt = false;
        char buf[1024];
        while (fgets(buf, 1024, fp)) {
            char *s = uncmt(buf, &in_cmt);
            if (s) {
                if (!s0)
                    s0 = se = new stringlist(s, 0);
                else {
                    se->next = new stringlist(s, 0);
                    se = se->next;
                }
            }
        }
        return (s0);
    }
}


// Function to parse a file, returning a list of lispnodes if
// successful.  If error, a message is returned in err.
//
lispnode *
cLispEnv::parseLisp(const char *filename, char **err)
{
    FILE *lp = 0;
    if (le_logging) {
        sLstr lstr;
        if (Log()->LogDirectory() && *Log()->LogDirectory()) {
            lstr.add(Log()->LogDirectory());
            lstr.add_c('/');
        }
        lstr.add(lstring::strip_path(filename));
        lstr.add("-lisp.log");

        if (filestat::create_bak(lstr.string())) {
            lp = fopen(lstr.string(), "w");
            if (lp) {
                fprintf(lp, "## %s  %s\n", lstring::strip_path(lstr.string()),
                    CD()->ifIdString());
            }
        }
        if (!lp) {
            static bool warned;
            if (!warned) {
                sLstr lstr_errs;
                lstr_errs.add("Can't open LISP log file");
                lstr_errs.add(", no logfile will be generated.\n");
                lstr_errs.add("Error: ");
                lstr_errs.add(filestat::error_msg());
                lstr_errs.add_c('\n');
                CD()->ifInfoMessage(IFMSG_POP_ERR, lstr_errs.string());

                warned = true;
            }
        }
    }

    *err = 0;
    if (!filename || !*filename) {
        *err = lstring::copy("null or empty file name");
        if (lp) {
            fprintf(lp, "## Error: %s.\n", *err);
            fclose(lp);
        }
        return (0);
    }
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        errset(err, "unable to open file %s", filename);
        if (lp) {
            fprintf(lp, "## Error: %s.\n", *err);
            fclose(lp);
        }
        return (0);
    }
    stringlist *sl = slurp_file(fp);
    fclose(fp);
    if (!sl) {
        errset(err, "no lines read from file %s", filename);
        if (lp) {
            fprintf(lp, "## Error: %s.\n", *err);
            fclose(lp);
        }
        return (0);
    }

    if (lp) {
        fprintf(lp, "## ---- string list ----\n");
        for (stringlist *s = sl; s; s = s->next)
            fprintf(lp, "%s\n", s->string);
        fprintf(lp, "## ---------------------\n\n");
    }

    char *s0 = stringlist::flatten(sl, " ");
    stringlist::destroy(sl);
    const char *s = s0;

    if (lp) {
        fprintf(lp, "## ---- tokens ----\n");
        char *tok;
        while ((tok = get_lisp_tok(&s)) != 0) {
            fprintf(lp, "%s\n", tok);
            delete [] tok;
        }
        s = s0;
        fprintf(lp, "## ---------------------\n\n");
    }

    lispnode *p0 = get_lisp_list(&s);

    if (lp) {
        fprintf(lp, "##--- regurgitation ---\n");
        for (lispnode *p = p0; p; p = p->next)
            lispnode::print(p, lp, false, false);

        fprintf(lp, "\n## ---------------------\n\n");
        fprintf(lp, "## Parse completed.\n\n");
        fclose(lp);
    }
    delete [] s0;
    return (p0);
}


lispnode *
cLispEnv::find_local_var(const char *name)
{
    for (lispnode *p = le_local_vars; p; p = p->next) {
        for (lispnode *q = p->args; q; q = q->next) {
            if (!strcmp(q->string, name))
                return (q->args);
        }
    }
    return (0);
}


// Return true is the name matches a variable in the table.
//
bool
cLispEnv::test_variable(const char *name)
{
    if (!le_global_vartab || (lispnode*)SymTab::get(le_global_vartab, name) ==
            (lispnode*)ST_NIL)
        return (false);
    return (true);
}


// Give back a lispnode obtained from new_temp_node() for reuse.
//
void
cLispEnv::recycle(lispnode *p)
{
    p->set_nil();
    p->next = le_recycle_list;
    le_recycle_list = p;
}


// Clear the variables and the temporary lispnode blocks.
//
void
cLispEnv::clear()
{
    delete le_global_vartab;
    le_global_vartab = 0;
    while (le_tln_blocks) {
        lnlist *ln = le_tln_blocks->next;
        delete [] le_tln_blocks->nodes;
        delete le_tln_blocks;
        le_tln_blocks = ln;
    }
    le_tln_index = 0;
    le_recycle_list = 0;
    while (le_local_vars)
        pop_local_vars();
}


//-----------------------------------------------------------------------
// Operators
//-----------------------------------------------------------------------

namespace {
    // Test/evaluate numerical arguments.
    //
    bool op_setup(lispnode *p0, lispnode *vl, lispnode *vr, char **err)
    {
        if (p0->lhs && !p0->lhs->eval(vl, err))
            return (false);
        if (vl->type != LN_NUMERIC) {

            // For unary operators, the lhs can be something unrelated
            // and should be ignored.  This is the case is
            // non-numeric, but if a string test if the string
            // contains a number.

            if (vl->type == LN_STRING) {
                const char *t = vl->string;
                double *dp = SPnum.parse(&t, true);
                if (dp) {
                    vl->type = LN_NUMERIC;
                    vl->value = *dp;
                }
            }
            if (vl->type != LN_NUMERIC) {
                const char *s = p0->string;
                if (s && (s[0] == '-' || s[0] == '+') && !s[1]) {
                    //unary
                    vl->type = LN_NUMERIC;
                    vl->value = 0.0;
                }
            }
            if (vl->type != LN_NUMERIC) {
                errset(err, "non-numeric lhs for %s", p0->string);
                return (false);
            }
        }
        if (p0->args && !p0->args->eval(vr, err))
            return (false);
        if (vr->type != LN_NUMERIC) {
            if (vr->type == LN_STRING) {
                const char *t = vr->string;
                double *dp = SPnum.parse(&t, true);
                if (dp) {
                    vr->type = LN_NUMERIC;
                    vr->value = *dp;
                }
            }
            if (vr->type != LN_NUMERIC) {
                errset(err, "non-numeric rhs for %s", p0->string);
                return (false);
            }
        }
        return (true);
    }


    bool expt(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        res->type = LN_NUMERIC;
        res->value = pow(vl.value, vr.value);
        return (true);
    }


    bool times(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        res->type = LN_NUMERIC;
        res->value = vl.value * vr.value;
        return (true);
    }


    bool quotient(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        res->type = LN_NUMERIC;
        res->value = vl.value / vr.value;
        return (true);
    }


    bool plus(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        res->type = LN_NUMERIC;
        res->value = vl.value + vr.value;
        return (true);
    }


    bool difference(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        res->type = LN_NUMERIC;
        res->value = vl.value - vr.value;
        return (true);
    }


    bool lessp(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        if (vl.value < vr.value) {
            res->type = LN_NUMERIC;
            res->value = 1.0;
        }
        else
            res->set_nil();
        return (true);
    }


    bool leqp(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        if (vl.value <= vr.value) {
            res->type = LN_NUMERIC;
            res->value = 1.0;
        }
        else
            res->set_nil();
        return (true);
    }


    bool greaterp(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        if (vl.value > vr.value) {
            res->type = LN_NUMERIC;
            res->value = 1.0;
        }
        else
            res->set_nil();
        return (true);
    }


    bool geqp(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (!op_setup(p0, &vl, &vr, err))
            return (false);
        if (vl.value >= vr.value) {
            res->type = LN_NUMERIC;
            res->value = 1.0;
        }
        else
            res->set_nil();
        return (true);
    }


    bool equal(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (p0->lhs && !p0->lhs->eval(&vl, err))
            return (false);
        if (p0->args && !p0->args->eval(&vr, err))
            return (false);
        bool istrue = false;
        if (vl.type == vr.type) {
            if (vl.type == LN_NUMERIC) {
                if (vl.value == vr.value)
                    istrue = true;
            }
            else if (vl.type == LN_STRING) {
                if (!strcmp(vl.string, vr.string))
                    istrue = true;
            }
            else if (vl.type == LN_NODE) {
                if (vl.arg_cnt() == vr.arg_cnt()) {
                    istrue = true;
                    lispnode r, t;
                    for (lispnode *pl = vl.args, *pr = vr.args; pl && pr;
                            pl = pl->next, pr = pr->next) {
                        t.lhs = pl;
                        t.args = pr;
                        if (!equal(&t, &r, err))
                            return (false);
                        if (r.is_nil()) {
                            istrue = false;
                            break;
                        }
                    }
                }
            }
        }
        if (istrue) {
            res->type = LN_NUMERIC;
            res->value = 1.0;
        }
        else
            res->set_nil();
        return (true);
    }


    bool nequal(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl, vr;
        if (p0->lhs && !p0->lhs->eval(&vl, err))
            return (false);
        if (p0->args && !p0->args->eval(&vr, err))
            return (false);
        bool istrue = true;
        if (vl.type == vr.type) {
            if (vl.type == LN_NUMERIC) {
                if (vl.value == vr.value)
                    istrue = false;
            }
            else if (vl.type == LN_STRING) {
                if (!strcmp(vl.string, vr.string))
                    istrue = false;
            }
            else if (vl.type == LN_NODE) {
                if (vl.arg_cnt() == vr.arg_cnt()) {
                    istrue = false;
                    lispnode r, t;
                    for (lispnode *pl = vl.args, *pr = vr.args; pl && pr;
                            pl = pl->next, pr = pr->next) {
                        t.lhs = pl;
                        t.args = pr;
                        if (!equal(&t, &r, err))
                            return (false);
                        if (r.is_nil()) {
                            istrue = true;
                            break;
                        }
                    }
                }
            }
        }
        if (istrue) {
            res->type = LN_NUMERIC;
            res->value = 1.0;
        }
        else
            res->set_nil();
        return (true);
    }


    bool and_node(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl;
        if (p0->lhs && !p0->lhs->eval(&vl, err))
            return (false);
        if (vl.is_nil()) {
            res->set_nil();
            return (true);
        }

        lispnode vr;
        if (p0->args && !p0->args->eval(&vr, err))
            return (false);
        if (vr.is_nil()) {
            res->set_nil();
            return (true);
        }

        res->type = LN_NUMERIC;
        res->value = 1.0;
        return (true);
    }


    bool or_node(lispnode *p0, lispnode *res, char **err)
    {
        lispnode vl;
        if (p0->lhs && !p0->lhs->eval(&vl, err))
            return (false);
        if (!vl.is_nil()) {
            res->type = LN_NUMERIC;
            res->value = 1.0;
            return (true);
        }

        lispnode vr;
        if (p0->args && !p0->args->eval(&vr, err))
            return (false);
        if (!vr.is_nil()) {
            res->type = LN_NUMERIC;
            res->value = 1.0;
            return (true);
        }

        res->set_nil();
        return (true);
    }


    // x:y ==> (x y)
    bool colon(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *vl = lispnode::get_env()->new_temp_node();
        lispnode *vr = lispnode::get_env()->new_temp_node();
        if (!op_setup(p0, vl, vr, err))
            return (false);
        vl->next = vr;
        res->type = LN_NODE;
        res->args = vl;
        return (true);
    }


    bool setq(lispnode *p0, lispnode*, char **err)
    {
        if (!p0->lhs)
            return (false);
        if (p0->lhs->type != LN_STRING) {
            *err = lstring::copy("setq: lhs not string type");
            return (false);
        }
        lispnode vr;
        if (!p0->args) {
            *err = lstring::copy("setq: no rhs");
            return (false);
        }
        if (!p0->args->eval(&vr, err))
            return (false);
        return (lispnode::get_env()->set_variable(p0->lhs->string, &vr, err));
    }
}


//-----------------------------------------------------------------------
// Math
//-----------------------------------------------------------------------

namespace {

    char *err_msg(const char *name, const char *err)
    {
        sLstr lstr;
        lstr.add(name);
        lstr.add(":  ");
        lstr.add(err);
        return (lstr.string_trim());
    }


    bool msetup(lispnode *p0, const char *fn, int argc, char **err)
    {
        if (p0->type != LN_NODE) {
            *err = err_msg(fn, "arg not a list");
            return (false);
        }
        int nargs = 0;
        for (lispnode *p = p0->args; p; p = p->next)
            nargs++;
        if (nargs < argc) {
            *err = err_msg(fn, "too few args");
            return (false);
        }
        if (nargs > argc) {
            *err = err_msg(fn, "too many args");
            return (false);
        }
        return (true);
    }


    bool abs(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "abs";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = fabs(v.value);
        res->set(&v);
        return (true);
    }


    bool sgn(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "sgn";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        if (v.value > 0.0)
            v.value = 1.0;
        else if (v.value < 0.0)
            v.value = -1.0;
        else
            v.value = 0.0;
        res->set(&v);
        return (true);
    }


    bool acos(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "acos";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::acos(v.value);
        res->set(&v);
        return (true);
    }


    bool asin(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "asin";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::asin(v.value);
        res->set(&v);
        return (true);
    }


    bool atan(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "atan";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::atan(v.value);
        res->set(&v);
        return (true);
    }

#ifdef HAVE_ACOSH

    bool acosh(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "acosh";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = acosh(v.value);
        res->set(&v);
        return (true);
    }


    bool asinh(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "asinh";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = asinh(v.value);
        res->set(&v);
        return (true);
    }


    bool atanh(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "atanh";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = atanh(v.value);
        res->set(&v);
        return (true);
    }

#endif // HAVE_ACOSH

    bool cos(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "cos";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::cos(v.value);
        res->set(&v);
        return (true);
    }


    bool cosh(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "cosh";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::cosh(v.value);
        res->set(&v);
        return (true);
    }


    bool exp(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "exp";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::exp(v.value);
        res->set(&v);
        return (true);
    }


    bool ln(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "ln";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::log(v.value);
        res->set(&v);
        return (true);
    }


    bool log(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "log";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        if (SIinterp::LogIsLog10())
            v.value = ::log10(v.value);
        else
            v.value = ::log(v.value);
        res->set(&v);
        return (true);
    }


    bool log10(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "log10";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::log10(v.value);
        res->set(&v);
        return (true);
    }


    bool sin(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "sin";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::sin(v.value);
        res->set(&v);
        return (true);
    }


    bool sinh(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "sinh";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::sinh(v.value);
        res->set(&v);
        return (true);
    }


    bool sqrt(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "sqrt";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::sqrt(v.value);
        res->set(&v);
        return (true);
    }


    bool tan(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "tan";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::tan(v.value);
        res->set(&v);
        return (true);
    }


    bool tanh(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "tanh";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::tanh(v.value);
        res->set(&v);
        return (true);
    }


    bool atan2(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "atan2";
        if (!msetup(p0, fn, 2, err))
           return (false);
        lispnode v1;
        if (!p0->args->eval(&v1, err))
            return (false);
        if (v1.type != LN_NUMERIC) {
            *err = err_msg(fn, "first argument not numeric");
            return (false);
        }
        lispnode v2;
        if (!p0->args->next->eval(&v2, err))
            return (false);
        if (v2.type != LN_NUMERIC) {
            *err = err_msg(fn, "second argument not numeric");
            return (false);
        }
        v1.value = ::atan2(v1.value, v2.value);
        res->set(&v1);
        return (true);
    }

    sRnd rnd;
    bool seeded;

    bool seed(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "seed";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        rnd.seed((unsigned int)v.value);
        seeded = true;
        res->set(&v);
        return (true);
    }


    // Return random number between 0 and 1.
    //
    bool random(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "random";
        if (!msetup(p0, fn, 0, err))
           return (false);
        if (!seeded) {
            rnd.seed(time(0));
            seeded = true;
        }
        lispnode v;
        v.type = LN_NUMERIC;
        v.value = rnd.random();
        res->set(&v);
        return (true);
    }


    bool gauss(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "gauss";
        if (!msetup(p0, fn, 0, err))
           return (false);
        if (!seeded) {
            rnd.seed(time(0));
            seeded = true;
        }
        lispnode v;
        v.type = LN_NUMERIC;
        v.value = rnd.gauss();
        res->set(&v);
        return (true);
    }


    bool floor(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "floor";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::floor(v.value);
        res->set(&v);
        return (true);
    }


    bool ceil(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "ceil";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = ::ceil(v.value);
        res->set(&v);
        return (true);
    }


    bool rint(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "rint";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        v.value = mmRnd(v.value);
        res->set(&v);
        return (true);
    }


    bool tint(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "rint";
        if (!msetup(p0, fn, 1, err))
           return (false);
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = err_msg(fn, "argument not numeric");
            return (false);
        }
        int i = (int)v.value;
        v.value = i;
        res->set(&v);
        return (true);
    }


    bool max(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "max";
        if (!msetup(p0, fn, 2, err))
           return (false);
        lispnode v1;
        if (!p0->args->eval(&v1, err))
            return (false);
        if (v1.type != LN_NUMERIC) {
            *err = err_msg(fn, "first argument not numeric");
            return (false);
        }
        lispnode v2;
        if (!p0->args->next->eval(&v2, err))
            return (false);
        if (v2.type != LN_NUMERIC) {
            *err = err_msg(fn, "second argument not numeric");
            return (false);
        }
        if (v2.value > v1.value)
            v1.value = v2.value;
        res->set(&v1);
        return (true);
    }


    bool min(lispnode *p0, lispnode *res, char **err)
    {
        const char *fn = "min";
        if (!msetup(p0, fn, 2, err))
           return (false);
        lispnode v1;
        if (!p0->args->eval(&v1, err))
            return (false);
        if (v1.type != LN_NUMERIC) {
            *err = err_msg(fn, "first argument not numeric");
            return (false);
        }
        lispnode v2;
        if (!p0->args->next->eval(&v2, err))
            return (false);
        if (v2.type != LN_NUMERIC) {
            *err = err_msg(fn, "second argument not numeric");
            return (false);
        }
        if (v2.value < v1.value)
            v1.value = v2.value;
        res->set(&v1);
        return (true);
    }
}


//-----------------------------------------------------------------------
// Flow Control
//-----------------------------------------------------------------------

namespace {
    // Main node for user scripts.  If a node is named "main" the
    // "arguments" will be evaluated here
    //
    bool main_node(lispnode *p0, lispnode *res, char **err)
    {
        for (lispnode *p = p0->args; p; p = p->next) {
            if (!p->eval(res, err))
                return (false);
        }
        return (true);
    }


    // Define a user-supplied procedure
    //
    bool procedure(lispnode *p0, lispnode*, char **err)
    {
        lispnode *p = p0->args;
        if (!p) {
            *err = lstring::copy("procedure: too few args");
            return (false);
        }
        if (p->type != LN_NODE || !p->string) {
            *err = lstring::copy("procedure: bad argument");
            return (false);
        }
        lispnode::get_env()->register_user_node(lispnode::dup_list(p));
        return (true);
    }


    // let( (local var list) ... )
    bool let(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *p = p0->args;
        if (!p) {
            *err = lstring::copy("procedure: too few args");
            return (false);
        }
        if (p->type != LN_NODE) {
            *err = lstring::copy("let: local var list not found");
            return (false);
        }
        lispnode *l0 = 0, *le = 0;
        for (lispnode *a = p->args; a; a = a->next) {
            lispnode r;
            if (!a->eval(&r, err))
                return (false);
            if (r.type != LN_STRING) {
                *err = lstring::copy("let: bad local var");
                return (false);
            }
            lispnode *x = lispnode::get_env()->new_temp_node();
            x->string = r.string;
            lispnode *n = lispnode::get_env()->new_temp_node();
            x->args = n;
            if (!l0)
                l0 = le = x;
            else {
                le->next = x;
                le = le->next;
            }
        }
        lispnode::get_env()->push_local_vars(l0);
        for (p = p->next; p; p = p->next) {
            if (!p->eval(res, err))
                return (false);
        }
        lispnode::get_env()->pop_local_vars();
        return (true);
    }


    // if( condition  then  action...  else  action... )
    bool if_node(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *c = p0->args;
        if (!c) {
            *err = lstring::copy("if: syntax error");
            return (false);
        }
        lispnode vc;
        if (!c->eval(&vc, err))
            return (false);

        if (!vc.is_nil()) {
            lispnode *p = c->next;
            if (!p->string || strcmp(p->string, "then")) {
                *err = lstring::copy("if: \"then\" missing");
                return (false);
            }
            p = p->next;
            for ( ; p; p = p->next) {
                if (p->string && !strcmp(p->string, "else"))
                    break;
                if (!p->eval(res, err))
                    return (false);
            }
        }
        else {
            lispnode *p = c->next;
            for ( ; p; p = p->next) {
                if (p->string && !strcmp(p->string, "else")) {
                    p = p->next;
                    break;
                }
            }
            if (p) {
                for ( ; p; p = p->next) {
                    if (!p->eval(res, err))
                        return (false);
                }
            }
        }
        return (true);
    }


    // when( condition action... )
    bool when_node(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *c = p0->args;
        if (!c) {
            *err = lstring::copy("when: syntax error");
            return (false);
        }
        lispnode vc;
        if (!c->eval(&vc, err))
            return (false);
        if (!vc.is_nil()) {
            for (lispnode *p = c->next; p; p = p->next) {
                if (!p->eval(res, err))
                    return (false);
            }
        }
        return (true);
    }


    // unless( condition action... )
    bool unless_node(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *c = p0->args;
        if (!c) {
            *err = lstring::copy("unless: syntax error");
            return (false);
        }
        lispnode vc;
        if (!c->eval(&vc, err))
            return (false);
        if (vc.is_nil()) {
            for (lispnode *p = c->next; p; p = p->next) {
                if (!p->eval(res, err))
                    return (false);
            }
        }
        return (true);
    }


    // case( what (value action..) ... )
    bool case_node(lispnode*, lispnode*, char**)
    {
        return (false);
    }


    // for( var start stop action... )
    bool for_node(lispnode*, lispnode*, char**)
    {
        return (false);
    }


    // foreach( var list action... )
    bool foreach_node(lispnode*, lispnode*, char**)
    {
        return (false);
    }
}


//-----------------------------------------------------------------------
// Lists
//-----------------------------------------------------------------------

namespace {
    // '( ... ) defines a list
    bool squote_node(lispnode *p0, lispnode *res, char**)
    {
        res->set(p0);
        return (true);
    }


    //  list( a b ... ) produces a list with var subst
    bool list(lispnode *p0, lispnode *res, char **err)
    {
        return (lispnode::eval_list(p0->args, res, err));
    }


    //  cons(elem list) = (elem list0 list1 ...)
    bool cons(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *ele = p0->args;
        if (!ele || !ele->next) {
            *err = lstring::copy("cons: too few args");
            return (false);
        }
        lispnode *lst = ele->next;

        lispnode r[2];
        if (!ele->eval(&r[0], err))
            return (false);
        if (!lst->eval(&r[1], err))
            return (false);
        res->set_nil();
        res->args = lispnode::get_env()->new_temp_node();
        res->args->set(&r[0]);
        if (r[1].type == LN_NODE)
            res->args->next = r[1].args;
        else {
            res->args->next = lispnode::get_env()->new_temp_node();
            res->args->next->set(&r[1]);
        }
        return (true);
    }


    //  append(list1 list2) = (list1 list2)
    bool append(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *ls1 = p0->args;
        if (!ls1 || !ls1->next) {
            *err = lstring::copy("append: too few args");
            return (false);
        }
        lispnode *ls2 = ls1->next;

        lispnode r[2];
        if (!ls1->eval(&r[0], err))
            return (false);
        if (!ls2->eval(&r[1], err))
            return (false);
        res->set_nil();
        if (r[0].type == LN_NODE)
            res->args = lispnode::get_env()->new_temp_copy(r[0].args);
        else {
            res->args = lispnode::get_env()->new_temp_node();
            res->args->set(&r[0]);
        }
        lispnode *a = res->args;
        while (a->next)
            a = a->next;
        if (r[1].type == LN_NODE)
            a->next = r[1].args;
        else {
            a->next = lispnode::get_env()->new_temp_node();
            a->next->set(&r[1]);
        }
        return (true);
    }


    //  car(list) = list0
    bool car(lispnode *p0, lispnode *res, char **err)
    {
        if (!p0->args) {
            *err = lstring::copy("car: too few args");
            return (false);
        }
        lispnode r;
        if (!p0->args->eval(&r, err))
            return (false);
        if (r.type == LN_NODE)
            res->set(r.args);
        else
            res->set(&r);
        return (true);
    }


    //  cdr(list) = (list1 list2 ...)
    bool cdr(lispnode *p0, lispnode *res, char **err)
    {
        if (!p0->args) {
            *err = lstring::copy("cdr: too few args");
            return (false);
        }
        lispnode r;
        if (!p0->args->eval(&r, err))
            return (false);
        res->set_nil();
        if (r.type == LN_NODE && r.args && r.args->next)
            res->args = r.args->next;
        return (true);
    }


    //  nth(num list) = list_num
    bool nth(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *pn = p0->args;
        if (!pn || !pn->next) {
            *err = lstring::copy("nth: too few args");
            return (false);
        }
        lispnode r;
        if (!pn->eval(&r, err))
            return (false);
        if (r.type != LN_NUMERIC) {
            *err = lstring::copy("nth: first arg not numeric");
            return (false);
        }
        int n = (int)r.value;
        if (n < 0) {
            res->set_nil();
            return (false);
        }

        lispnode l;
        if (!pn->next->eval(&l, err))
            return (false);
        res->set_nil();
        if (l.type == LN_NODE) {
            lispnode *a = l.args;
            while (a && n--)
                a = a->next;
            if (a && n == -1)
                res->set(a);
        }
        else if (n == 0)
            res->set(&l);
        return (true);
    }


    //  member(elem list)
    bool member(lispnode *p0, lispnode *res, char **err)
    {
        lispnode *ele = p0->args;
        if (!ele || !ele->next) {
            *err = lstring::copy("member: too few args");
            return (false);
        }
        lispnode *lst = ele->next;

        lispnode r[2];
        if (!ele->eval(&r[0], err))
            return (false);
        if (!lst->eval(&r[1], err))
            return (false);

        res->set_nil();
        if (r[1].type == LN_NODE) {
            lispnode v;
            v.lhs = &r[0];
            for (lispnode *l = r[1].args; l; l = l->next) {
                v.args = l;
                lispnode rt;
                if (!equal(&v, &rt, err))
                    return (false);
                if (!rt.is_nil()) {
                    res->type = LN_NUMERIC;
                    res->value = 1.0;
                    break;
                }
            }
        }
        else {
            lispnode v;
            v.lhs = &r[0];
            v.args = &r[1];
            lispnode rt;
            if (!equal(&v, &rt, err))
                return (false);
            if (!rt.is_nil()) {
                res->type = LN_NUMERIC;
                res->value = 1.0;
            }
        }
        return (true);
    }


    //  length(list)
    bool length(lispnode *p0, lispnode *res, char **err)
    {
        if (!p0->args) {
            *err = lstring::copy("length: too few args");
            return (false);
        }
        lispnode r;
        if (!p0->args->eval(&r, err))
            return (false);
        if (r.type == LN_NODE)
            res->value = r.arg_cnt();
        else
            res->value = 1;
        res->type = LN_NUMERIC;
        return (true);
    }


    bool xCoord(lispnode *p0, lispnode *res, char **err)
    {
        if (p0->type != LN_NODE) {
            *err = lstring::copy("xCoord: arg not a list");
            return (false);
        }
        if (!p0->args) {
            *err = lstring::copy("xCoord: too few args");
            return (false);
        }
        lispnode v;
        if (!p0->args->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = lstring::copy("xCoord: first list element not numeric");
            return (false);
        }
        res->set(&v);
        return (true);
    }


    bool yCoord(lispnode *p0, lispnode *res, char **err)
    {
        if (p0->type != LN_NODE) {
            *err = lstring::copy("yCoord: arg not a list");
            return (false);
        }
        if (!p0->args || !p0->args->next) {
            *err = lstring::copy("yCoord: too few args");
            return (false);
        }
        lispnode v;
        if (!p0->args->next->eval(&v, err))
            return (false);
        if (v.type != LN_NUMERIC) {
            *err = lstring::copy("yCoord: second list element not numeric");
            return (false);
        }
        res->set(&v);
        return (true);
    }
}


//-----------------------------------------------------------------------
// The lispnode functions
//-----------------------------------------------------------------------

cLispEnv *lispnode::lisp_env = 0;


lispnode::lispnode()
{
    type = LN_NODE;
    args = 0;
    lhs = 0;
    next = 0;
    string = 0;
    value = 0.0;
}


lispnode::lispnode(LN_TYPE ltype, char *str)
{
    type = ltype;
    args = 0;
    lhs = 0;
    next = 0;
    string = str;
    value = 0.0;
    if (type == LN_NUMERIC) {
        const char *t = str;
        double *dp = SPnum.parse(&t, false);
        value = dp ? *dp : 0.0;
    }
    else if (type == LN_STRING) {
        if (!strcmp(str, "nil")) {
            type = LN_NODE;
            delete [] string;
            string = 0;
        }
        else if (!strcmp(str, "t")) {
            type = LN_NUMERIC;
            value = 1.0;
            delete [] string;
            string = 0;
        }
    }
}


// Static function.
// Duplicate a node, and all structure.
//
lispnode *
lispnode::dup(const lispnode *thisn)
{
    if (!thisn)
        return (0);
    lispnode *n = new lispnode;
    n->type = thisn->type;
    n->string = lstring::copy(thisn->string);
    n->args = dup_list(thisn->args);
    n->lhs = dup(thisn->lhs);
    n->value = thisn->value;
    return (n);
}


// Static function.
// Duplicate a list, and all structure
//
lispnode *
lispnode::dup_list(const lispnode *thisn)
{
    lispnode *l0 = 0, *le = 0;
    for (const lispnode *p = thisn; p; p = p->next) {
        if (!l0)
            l0 = le = dup(p);
        else {
            le->next = dup(p);
            le = le->next;
        }
    }
    return (l0);
}


// Static function.
// Free a list of lispnodes, including contents.  The lispnode has no
// destructor.  Call this *only* for nodes returned from the parser,
// or otherwise known to be safe to free.
//
// Note:  LispEnv::clear() should be called after freeing a tree that
// has been evaluated, since the variables contain pointer to tree
// data.
//
void
lispnode::destroy(const lispnode *thisn)
{
    const lispnode *pn;
    for (const lispnode *p = thisn; p; p = pn) {
        pn = p->next;
        if (p->type != LN_OPER) {
            // for opers, this is ->Loper::name
            delete [] p->string;
        }
        destroy(p->lhs);
        destroy(p->args);
        delete p;
    }
}


namespace {
    inline void
    newline(FILE *fp, int idlev)
    {
        putc('\n', fp);
        while (idlev--)
            putc(' ', fp);
    }
}


// Static function.
// Print the listing from the tree (recursive).  Returns true if a linefeed
// was emitted.
//
bool
lispnode::print(const lispnode *thisn, FILE *fp, int idlev, bool nonl)
{
    if (!thisn) {
        fprintf(fp, "\nInternal error: NULL NODE!\n");
        return (true);
    }
    bool lf = false;
    if (thisn->type == LN_NODE) {
        if (thisn->is_nil())
            fprintf(fp, " nil");
        else {
            if (thisn->string) {
                if (nonl)
                    fprintf(fp, " %s(", thisn->string);
                else {
                    newline(fp, idlev);
                    fprintf(fp, "%s(", thisn->string);
                    lf = true;
                }
            }
            else {
                if (nonl)
                    fprintf(fp, " (");
                else {
                    newline(fp, idlev);
                    fprintf(fp, "(");
                    lf = true;
                }
            }
            bool alf = false;
            for (lispnode *p = thisn->args; p; p = p->next) {
                if (print(p, fp, idlev+1, nonl))
                    alf = true;
            }
            if (!alf || nonl)
                fprintf(fp, " )");
            else {
                newline(fp, idlev);
                fprintf(fp, ")");
            }
        }
    }
    else if (thisn->type == LN_STRING)
        fprintf(fp, " %s", thisn->string);
    else if (thisn->type == LN_NUMERIC) {
        if (thisn->string)
            fprintf(fp, " %s", thisn->string);
        else
            fprintf(fp, " %g", thisn->value);
    }
    else if (thisn->type == LN_OPER) {
        if (nonl) {
            if (thisn->lhs)
                print(thisn->lhs, fp, 0, true);
            fprintf(fp, " %s", thisn->string);
        }
        else {
            newline(fp, idlev);
            if (thisn->lhs)
                print(thisn->lhs, fp, 0, true);
            fprintf(fp, " %s", thisn->string);
            lf = true;
        }
        print(thisn->args, fp, 0, true);
    }
    else if (thisn->type == LN_QSTRING)
        fprintf(fp, " \"%s\"", thisn->string);
    return (lf);
}


// Static function.
void
lispnode::print(const lispnode *thisn, sLstr *lstr)
{
    if (!thisn)
        return;
    if (thisn->type == LN_NODE) {
        if (thisn->is_nil())
            lstr->add(" nil");
        else {
            if (thisn->string) {
                lstr->add_c(' ');
                lstr->add(thisn->string);
                lstr->add_c('(');
            }
            else {
                lstr->add_c(' ');
                lstr->add_c('(');
            }
            for (const lispnode *p = thisn->args; p; p = p->next)
                print(p, lstr);
            lstr->add_c(')');
        }
    }
    else if (thisn->type == LN_STRING) {
        lstr->add_c(' ');
        lstr->add(thisn->string);
    }
    else if (thisn->type == LN_NUMERIC) {
        lstr->add_c(' ');
        if (thisn->string)
            lstr->add(thisn->string);
        else
            lstr->add_g(thisn->value);
    }
    else if (thisn->type == LN_OPER) {
        if (thisn->lhs) {
            print(thisn->lhs, lstr);
            lstr->add_c(' ');
        }
        lstr->add(thisn->string);
        print(thisn->args, lstr);
    }
    else if (thisn->type == LN_QSTRING) {
        lstr->add_c(' ');
        lstr->add_c('"');
        lstr->add(thisn->string);
        lstr->add_c('"');
    }
}


// Static function.
// Evaluate nargs elements, returning the results in the res array
// which must have size nargs or larger.  The return value is the
// number of list elements actually evaluated, or -1 if error, in
// which case a message should be returned in err.
//
int
lispnode::eval_list(lispnode *thisn, lispnode *res, int nargs, char **err)
{
    int cnt = 0;
    for (lispnode *p = thisn; p && cnt < nargs; p = p->next) {

        // Check for a unary operator that follows something
        // non-numeric.  In this case there are really two arguments: 
        // the lhs, and the operator-modified rhs.

        if (p->type == LN_OPER && (*p->string == '-' || *p->string == '+')) {
            if (p->lhs && p->lhs->type != LN_NUMERIC) {
                if (!p->lhs->eval(&res[cnt], err))
                    return (-1);
                if (res[cnt].type != LN_NUMERIC) {
                    double *dp = 0;
                    if (res[cnt].type == LN_STRING) {
                        const char *t = res[cnt].string;
                        dp = SPnum.parse(&t, true);
                    }
                    if (!dp)
                        cnt++;
                }
            }
        }
        if (!p->eval(&res[cnt], err))
            return (-1);
        cnt++;
    }
    return (cnt);
}


// Static function.
// Evaluate the list, returned as an arg list of res, temporary storage.
//
bool
lispnode::eval_list(lispnode *thisn, lispnode *res, char **err)
{
    lispnode *l0 = 0, *le = 0;
    for (lispnode *p = thisn; p; p = p->next) {
        if (!l0)
            l0 = le = lisp_env->new_temp_node();
        else {
            le->next = lisp_env->new_temp_node();
            le = le->next;
        }
        if (!p->eval(le, err))
            return (false);
    }
    res->type = LN_NODE;
    res->string = 0;
    res->args = l0;
    return (true);
}


// Evaluation function for a lispnode.  If error, return false with a
// message in err.  Otherwise, return true with result in res.
//
bool
lispnode::eval(lispnode *res, char **err)
{
    *err = 0;
    if (!lisp_env) {
        *err = lstring::copy("no execution environment");
        return (false);
    }

    if (type == LN_NODE) {
        if (is_nil())
            res->set_nil();
        else if (string) {
            nodefunc func = lisp_env->find_func(string);
            if (func) {
                if (!(*func)(this, res, err))
                    return (false);
            }
            else {
                int ret = eval_user_node(res, err);
                if (ret < 0)
                    return (false);
                if (ret == 0) {
                    ret = eval_xic_node(res, err);
                    if (ret < 0)
                        return (false);
                    if (ret == 0) {
                        errset(err, "unknown function %s", string);
                        return (false);
                    }
                }
            }
        }
        else {
            if (arg_cnt() == 1) {
                // Just one item in the parentheses, return the item.
                // Need this for arithemetic expressions
                if (!args->eval(res, err))
                    return (false);
            }
            else {
                if (!eval_list(args, res, err))
                    return (false);
            }
        }
    }
    else if (type == LN_STRING) {
        if (!lisp_env->get_variable(res, string)) {
            res->type = LN_STRING;
            res->string = string;
        }
    }
    else if (type == LN_NUMERIC) {
        res->type = LN_NUMERIC;
        res->value = value;
    }
    else if (type == LN_OPER) {
        nodefunc func = 0;
        for (sLoper *op = Lopers; op->name; op++) {
            if (op->string == string) {
                func = lisp_env->find_func(op->name);
                break;
            }
        }
        if (!func) {
            errset(err, "no function for operator %s", string);
            return (false);
        }
        if (!(*func)(this, res, err))
            return (false);
    }
    else if (type == LN_QSTRING) {
        res->type = LN_STRING;
        res->string = string;
    }
    else {
        *err = lstring::copy("internal, bad node type");
        return (false);
    }
    return (true);
}


// Evaluate the present function node as an Xic function.  Return values
// are -1 if fatal error, 0 if node name not found, 1 if ok.
//
int
lispnode::eval_xic_node(lispnode *res, char **err)
{
    char funcname[64];
    strcpy(funcname, string);
    if (islower(*funcname))
        *funcname = toupper(*funcname);

    SIptfunc *func = SIparse()->function(funcname);
    if (!func)
        return (0);
    int ac = arg_cnt();
    if (ac > MAXARGC) {
        errset(err, "%s: too many args", string);
        return (-1);
    }

    lispnode r[MAXARGC];
    if (eval_list(args, r, ac, err) < 0)
        return (-1);

    Variable v[MAXARGC+1];
    for (int i = 0; i < ac; i++) {
        if (!r[i].ln_to_var(&v[i], err))
            return (-1);
    }
    v[ac].type = TYP_ENDARG;

    Variable vr;
    bool ret = (*func->func())(&vr, v, SI()->LexprCx());
    if (ret != OK) {
        errset(err, "execution failed for %s", string);
        return (-1);
    }
    if (!res->var_to_ln(&vr, err))
        return (-1);
    return (1);
}


// Evaluate the present function node as a user procedure.  Return
// values are -1 if fatal error, 0 if node name not found, 1 if ok.
//
int
lispnode::eval_user_node(lispnode *res, char **err)
{
    lispnode *pf = lisp_env->find_user_node(string);
    if (!pf)
        return (0);

    // The first node of pf contains the arguemnt list, the remaining
    // nodes are the function body
    lispnode *aa = args;  // actual args
    lispnode *fa = 0, *fe = 0;
    bool optional = false;

    if (!pf->next)
        // empty function body
        return (1);

    // Set up the arguments
    for (lispnode *p =  pf->args; p; p = p->next) {
        if (p->type == LN_STRING || p->type == LN_QSTRING) {
            if (!strcmp(p->string, "@rest")) {
                p = p->next;
                if (!p || (p->type != LN_STRING && p->type != LN_QSTRING)) {
                    *err = lstring::copy("no or bad arg following \"@rest\"");
                    return (-1);
                }
                lispnode *n = lisp_env->new_temp_node();
                if (!eval_list(aa, n, err))
                    return (-1);
                lispnode *x = lisp_env->new_temp_node();
                x->args = n;
                x->string = p->string;
                if (!fa)
                    fa = fe = x;
                else {
                    fe->next = x;
                    fe = fe->next;
                }
                break;
            }
            else if (!strcmp(p->string, "@optional")) {
                optional = true;
                continue;
            }
            else if (!strcmp(p->string, "@key")) {
                *err = lstring::copy("can't accept \"@key\" yet");
                return (-1);
            }
            else {
                lispnode *n = lisp_env->new_temp_node();
                if (!aa->eval(n, err))
                    return (-1);
                lispnode *x = lisp_env->new_temp_node();
                x->args = n;
                x->string = p->string;
                if (!fa)
                    fa = fe = x;
                else {
                    fe->next = x;
                    fe = fe->next;
                }
                aa = aa->next;
                continue;
            }
        }
        else if (p->type == LN_NODE) {
            if (optional) {
                lispnode v[2];
                int ret = eval_list(p->args, v, 2, err);
                if (ret < 0)
                    return (-1);
                if (ret < 2 || v[0].type != LN_STRING) {
                    errset(err, "bad default spec in %s arg list", pf->string);
                    return (-1);
                }
                lispnode *n = lisp_env->new_temp_node();
                if (aa) {
                    if (!aa->eval(n, err))
                        return (-1);
                    aa = aa->next;
                }
                else
                    n->set(&v[1]);
                lispnode *x = lisp_env->new_temp_node();
                x->args = n;
                x->string = v[0].string;
                if (!fa)
                    fa = fe = x;
                else {
                    fe->next = x;
                    fe = fe->next;
                }
                continue;
            }
            return (-1);
        }
    }
    lisp_env->push_local_vars(fa);
    int ret = 1;
    for (lispnode *p = pf->next; p; p = p->next) {
        if (!p->eval(res, err)) {
            ret = -1;
            break;
        }
    }
    lisp_env->pop_local_vars();
    return (ret);
}


// Set the variable from this.
//
bool
lispnode::ln_to_var(Variable *v, char **err)
{
    if (type == LN_NODE) {
        if (is_nil()) {
            v->type = TYP_SCALAR;
            v->content.value = 0.0;
        }
        else {
            *err = lstring::copy("ln_to_var: can't handle arrays yet");
            return (false);
        }
    }
    else if (type == LN_STRING) {
        v->type = TYP_STRING;
        v->content.string = string;
    }
    else if (type == LN_NUMERIC) {
        v->type = TYP_SCALAR;
        v->content.value = value;
    }
    else {
        *err = lstring::copy("ln_to_var: oddball data type");
        return (false);
    }
    return (true);
}


// Set this from the variable.
//
bool
lispnode::var_to_ln(Variable *v, char **err)
{
    if (v->type == TYP_NOTYPE || v->type == TYP_STRING) {
        type = LN_STRING;
        string = v->content.string;
    }
    else if (v->type == TYP_SCALAR) {
        type = LN_NUMERIC;
        value = v->content.value;
    }
    else if (v->type == TYP_ARRAY) {
        *err = lstring::copy("var_to_ln: can't handle arrays yet");
        return (false);
    }
    else {
        *err = lstring::copy("var_to_ln: oddball data type");
        return (false);
    }
    return (true);
}
// End of lispnode functions

