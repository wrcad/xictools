
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "parser.h"
#include "cshell.h"
#include "commands.h"
#include "spnumber/paramsub.h"
#include "miscutil/lstring.h"
#include "ginterf/graphics.h"


// User-defined functions. The user defines the function with
//   define func(arg1, arg2, arg3) <expression involving args...>
// Note that we have to take some care to distinguish between
// functions with the same name and different arities.
//
// Multiple UDF contexts are supported.  A "shell context" is used
// with the shell define command.  Other contexts can be circuit or
// plot related, or set during certain operations such as subcircuit
// expansion.  These will override lower contexts, but searches for
// function matches traverse all contexts.


// The list of UDF contexts.
//
struct udf_list
{
    udf_list(cUdf *u, udf_list *n) { ul_udfs = u; ul_next = n; }
    cUdf *udfdb()       { return (ul_udfs); }
    udf_list *next()    { return (ul_next); }

private:
    cUdf *ul_udfs;
    udf_list *ul_next;
};


// A text element, used for printing.
//
struct udf_text
{
    udf_text(sUdFunc*);

    ~udf_text()
        {
            delete [] name;
            delete [] text;
        }

    char *name;
    char *text;
    int arity;
    bool is_shell;
};


namespace {
    cUdf shell_udf;             // Shell user-defined functions.
    udf_list *tran_udfs = 0;    // Transient UDF's
}


// Generator to return all UDFs in scope in correct hierarchical
// order.  First come the transient UDFs, then the current cell UDF,
// finally the shell UDF.
//
struct udf_gen
{
    udf_gen()
        {
            reset();
        }

    void reset()
        {
            ug_next = 0;
            ug_list = tran_udfs;
            ug_cc_done = false;
            advance();
        }

    cUdf *next()
        {
            cUdf *ret = ug_next;
            if (ret)
                advance();
            return (ret);
        }

    cUdf *cur()
        {
            return (ug_next);
        }

private:
    void advance()
        {
            while (ug_list) {
                if (ug_list->udfdb()) {
                    ug_next = ug_list->udfdb();
                    ug_list = ug_list->next();
                    return;
                }
                ug_list = ug_list->next();
            }
            if (!ug_cc_done) {
                ug_cc_done = true;
                if (Sp.CurCircuit() && Sp.CurCircuit()->defines()) {
                    ug_next = Sp.CurCircuit()->defines();
                    return;
                }
            }
            if (ug_next == &shell_udf) {
                ug_next = 0;
                return;
            }
            ug_next = &shell_udf;
        }

    cUdf *ug_next;
    udf_list *ug_list;
    bool ug_cc_done;
};


namespace {
    // Comparison function for printing sort.
    //
    int udfcmp(const void *a, const void *b)
    {
        char buf1[256], buf2[256];
        udf_text *t1 = *(udf_text**)a;
        udf_text *t2 = *(udf_text**)b;
        char *s = buf1;
        const char *t = t1->name;
        while (*t && !isspace(*t) && *t != '(')
            *s++ = *t++;
        *s = 0;
        s = buf2;
        t = t2->name;
        while (*t && !isspace(*t) && *t != '(')
            *s++ = *t++;
        *s = 0;
        int i = strcmp(buf1, buf2);
        if (i)
            return (i);
        i = t1->arity - t2->arity;
        if (i)
            return (i);
        i = t1->is_shell - t2->is_shell;
        return (i);
    }


    // Print matching or all function definitions.
    //
    void print_udfs(const char *name)
    {
        char buf[BSIZE_SP];
        sHtab *tab = new sHtab(false);
        udf_gen gen;
        cUdf *udfdb;
        while ((udfdb = gen.next()) != 0) {
            if (udfdb == &shell_udf)
                break;
            if (!udfdb->table())
                continue;
            if (name && *name) {
                sUdFunc *udf = (sUdFunc*)sHtab::get(udfdb->table(), name);
                for (sUdFunc *u = udf; u; u = u->next()) {
                    snprintf(buf, sizeof(buf), "%s:%d", name, u->argc());
                    udf_text *otxt = (udf_text*)sHtab::get(tab, buf);
                    if (otxt)
                        continue;
                    udf_text *txt = new udf_text(u);
                    tab->add(buf, txt);
                }
            }
            else {
                sHgen hgen(udfdb->table());
                sHent *h;
                while ((h = hgen.next()) != 0) {
                    for (sUdFunc *u = (sUdFunc*)h->data(); u; u = u->next()) {
                        snprintf(buf, sizeof(buf), "%s:%d", h->name(),
                            u->argc());
                        udf_text *otxt = (udf_text*)sHtab::get(tab, buf);
                        if (otxt)
                            continue;
                        udf_text *txt = new udf_text(u);
                        tab->add(buf, txt);
                    }
                }
            }
        }
        if (shell_udf.table()) {
            if (name && *name) {
                sUdFunc *udf = (sUdFunc*)sHtab::get(shell_udf.table(), name);
                for (sUdFunc *u = udf; u; u = u->next()) {
                    snprintf(buf, sizeof(buf), "%s:%dS", name, u->argc());
                    udf_text *otxt = (udf_text*)sHtab::get(tab, buf);
                    if (otxt)
                        continue;
                    udf_text *txt = new udf_text(u);
                    txt->is_shell = true;
                    tab->add(buf, txt);
                }
            }
            else {
                sHgen hgen(shell_udf.table());
                sHent *h;
                while ((h = hgen.next()) != 0) {
                    for (sUdFunc *u = (sUdFunc*)h->data(); u; u = u->next()) {
                        snprintf(buf, sizeof(buf), "%s:%dS", h->name(),
                            u->argc());
                        udf_text *otxt = (udf_text*)sHtab::get(tab, buf);
                        if (otxt)
                            continue;
                        udf_text *txt = new udf_text(u);
                        txt->is_shell = true;
                        tab->add(buf, txt);
                    }
                }
            }
        }

        if (!tab->allocated()) {
            TTY.send("No functions defined.\n");
            delete tab;
            return;
        }

        udf_text **ary = new udf_text*[tab->allocated()];
        sHgen hgen(tab, true);
        sHent *h;
        int cnt = 0;
        while ((h = hgen.next()) != 0) {
            ary[cnt++] = (udf_text*)h->data();
            delete h;
        }
        delete tab;

        qsort(ary, cnt, sizeof(udf_text*), udfcmp);

        for (int i = 0; i < cnt; i++) {
            udf_text *txt = ary[i];
            if (!txt->is_shell)
                TTY.send("* ");
            else
                TTY.send("  ");
            TTY.send(txt->name);
            TTY.send(" = ");
            TTY.send(txt->text);
            TTY.send("\n");
            delete txt;
        }
        delete [] ary;
    }


    // Count the arguments in the comma-separated list passed.
    //
    inline int
    count_args(const pnode *args)
    {
        if (!args)
            return (0);
        int arity = 1;
        for (const pnode *tp = args;
                tp && tp->oper() && (tp->oper()->optype() == TT_COMMA);
                tp = tp->right())
            arity++;
        return (arity);
    }
}


// Set up a function definition in the shell UDF context.
//
void
CommandTab::com_define(wordlist *wlist)
{
    char *lhs, *body;
    if (!shell_udf.parse(wlist, &lhs, &body))
        return;
    if (!body) {
        print_udfs(lhs);
        delete [] lhs;
        return;
    }

    shell_udf.define(lhs, body);
    delete [] lhs;
    delete [] body;
}


// Undefine function definition(s) in the shell UDF context.
//
void
CommandTab::com_undefine(wordlist *wlist)
{
    shell_udf.clear(wlist);
}
// End of CommandTab functions.


// Above the "shell" level of defines, there are defines associated
// with the current circuit.  There are additional transient levels
// that are used when expanding subcircuits.  Functions defined within
// a subcircuit or on the .subckt line are pushed when the subckt is
// expanded.

// Push a new transient UDF, return the current top-of-stack.
//
cUdf *
IFsimulator::PushUserFuncs(cUdf *newudf)
{
    if (!newudf)
        newudf = new cUdf;
    tran_udfs = new udf_list(newudf, tran_udfs);
    return (newudf);
}


// Unlink and return the present pushed-to transient UDF context,
// return to the previous context.
//
// The UDF is *not* destroyed, but is returned.
//
cUdf *
IFsimulator::PopUserFuncs()
{ 
    cUdf *udf = 0;
    if (tran_udfs) {
        udf_list *lx = tran_udfs;
        tran_udfs = tran_udfs->next();
        udf = lx->udfdb();
        delete lx;
    }
    return (udf);
}


// Define a function in the current UDF context.
//
bool
IFsimulator::DefineUserFunc(wordlist *wlist)
{
    udf_gen gen;
    return (gen.cur()->define(wlist));
}


// Define a function is the current UDF context.
//
bool
IFsimulator::DefineUserFunc(const char *fname, const char *body)
{
    udf_gen gen;
    return (gen.cur()->define(fname, body));
}


// Return true if there is a user-defined function with the given
// arity.  If arity < 0, search for name only.  All contexts are
// searched.
//
bool
IFsimulator::IsUserFunc(const char *name, int arity)
{
    udf_gen gen;
    cUdf *udfdb;
    while ((udfdb = gen.next()) != 0) {
        if (udfdb->is_defined(name, arity))
            return (true);
    }
    return (false);
}


// As above, but match the number of arguments.
//
bool
IFsimulator::IsUserFunc(const char *name, const pnode *args)
{
    int arity = count_args(args);
    udf_gen gen;
    cUdf *udfdb;
    while ((udfdb = gen.next()) != 0) {
        if (udfdb->is_defined(name, arity))
            return (true);
    }
    return (false);
}


// Get the macro text (copied) and the args (not copied) and return
// true if name/arity match a defined function in any context.
//
bool
IFsimulator::UserFunc(const char *name, int arity, char** text,
    const char **args)
{
    udf_gen gen;
    cUdf *udfdb;
    while ((udfdb = gen.next()) != 0) {
        if (udfdb->get_macro(name, arity, text, args))
            return (true);
    }
    return (false);
}


// Return a copy of the parse tree under the given name, with the args
// copied in for the formal args.  All UDF contexts are searched.
//
pnode *
IFsimulator::GetUserFuncTree(const char *name, const pnode *args)
{
    int nargs = count_args(args);
    udf_gen gen;
    cUdf *udfdb;
    while ((udfdb = gen.next()) != 0) {
        sUdFunc *udf = udfdb->find(name, nargs);
        if (udf)
            return (udfdb->get_tree(udf, args));
    }
    gen.reset();
    while ((udfdb = gen.next()) != 0) {
        sUdFunc *udf = udfdb->find(name, -1);
        if (udf) {
            GRpkg::self()->ErrPrintf(ET_WARN,
                "the user-defined function %s has %d args.\n",
                udf->name(), udf->argc());
            break;
        }
    }
    return (0);
}


// Test if pfunc calls a transient UDF.  If so, parameter-substitute
// the UDF text, and save the function in the current cell UDF table
// under a new name, and update that name in pfunc.
//
bool
IFsimulator::TestAndPromote(pnode *pfunc, const sParamTab *ptab,
    sMacroMapTab *mtab)
{
    if (!pfunc || !pfunc->func() || !ft_curckt)
        return (true);
    cUdf *ccdb = ft_curckt->defines();
    if (!ccdb)
        return (true);
    const char *fname = pfunc->func()->name();
    int nargs = count_args(pfunc->left());

    if (mtab) {
        const char *newname = mtab->get(fname, nargs);
        if (newname) {
            // We've already dealt with this one.
            delete [] pfunc->func()->name();
            pfunc->func()->set_name(lstring::copy(newname));
            return (true);
        }
    }

    for (udf_list *ul = tran_udfs; ul; ul = ul->next()) {
        if (ul->udfdb() == ccdb)
            continue;

        sUdFunc *udf = ul->udfdb()->find(fname, nargs);
        if (udf) {

            pnode *ptmp = udf->tree()->copy();
            if (!ptmp)
                return (false);
            ptmp->promote_macros(ptab, mtab);
            char *text = ptmp->get_string();
            delete ptmp;

            if (!text)
                return (false);

            // Param expand, line_subst mode.
            if (ptab)
                ptab->param_subst_all(&text);

            if (ccdb->define_unique(&fname, udf->name(), text)) {
                if (mtab) {
                    // The table data pointer will point into the
                    // pnode's sFunc record.  This is ok, the table is
                    // destroyed when were through promoting macros,
                    // before any nodes are destroyed.

                    mtab->add(pfunc->func()->name(), nargs, fname);
                }
                delete [] pfunc->func()->name();
                pfunc->func()->set_name(fname);
                delete [] text;
                return (true);
            }
            delete [] text;
            break;
        }
    }
    return (false);
}
// End of IFsimulator functions.


cUdf::~cUdf()
{
    if (ud_tab) {
        sHgen gen(ud_tab, true);
        sHent *h;
        while ((h = gen.next()) != 0) {
            sUdFunc *udf = (sUdFunc*)h->data();
            while (udf) {
                sUdFunc *ux = udf;
                udf = udf->next();
                delete ux;
            }
            delete h;
        }
        delete ud_tab;
    }
    if (ud_promo_tab) {
        sHgen gen(ud_promo_tab, true);
        sHent *h;
        while ((h = gen.next()) != 0) {
            delete [] (char*)h->data();
            delete h;
        }
        delete ud_promo_tab;
    }
}


// Parse a function definition wordlist, return formatted left side
// and body specification strings.
//
bool
cUdf::parse(wordlist *wlist, char **plhs, char **pbody) const
{
    // Allow these chars in argument tokens, in addition to the
    // alpha-numerics.
    static const char *argchars = "_#@$";

    if (!plhs || !pbody)
        return (false);
    *plhs = 0;
    *pbody = 0;

    if (!wlist)
        return (true);
    char *buf = wordlist::flatten(wlist);
    if (!buf)
        return (true);

    sLstr lstr;
    int pcnt = 0;
    char *t = buf;
    for ( ; *t; t++) {
        if (isspace(*t))
            continue;
        if (isalnum(*t) || strchr(argchars, *t)) {
            lstr.add_c(*t);
            continue;
        }
        if (*t == '(') {
            pcnt++;
            if (pcnt == 1)
                lstr.add_c(*t);
            continue;
        }
        if (*t == ')') {
            pcnt--;
            if (pcnt == 0) {
                lstr.add_c(*t);
                t++;
                break;
            }
            if (pcnt > 0)
                continue;
            if (pcnt < 0) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad parentheses nesting in call template.\n", buf);
                delete [] buf;
                return (false);
            }
        }
        if (*t == ',') {
            if (pcnt == 1) {
                lstr.add_c(*t);
                continue;
            }
        }
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "bad character '%c' found in call template.\n", buf);
        delete [] buf;
        return (false);
    }

    // Allow an equal sign, i.e.,  f(...) [=] expr
    while (isspace(*t) || *t == '=')
        t++;

    *plhs = lstr.string_trim();
    if (*t)
        *pbody = lstring::copy(t);
    delete [] buf;
    return (true);
}


// Define a function.
//
bool
cUdf::define(wordlist *wlist)
{
    char *lhs, *body;
    if (!parse(wlist, &lhs, &body))
        return (false);

    if (!body || !lhs) {
        delete [] body;
        delete [] lhs;
        return (false);
    }
    bool ret = define(lhs, body);
    delete [] body;
    delete [] lhs;
    return (ret);
}


// Define a function.
//
bool
cUdf::define(const char *fname, const char *body)
{
    if (!fname || !*fname)
        return (false);
    if (!body || !*body)
        return (false);

    // Check to see if this is a valid name for a function (i.e, 
    // there isn't a predefined function of the same name).
    //
    char buf[BSIZE_SP];
    strcpy(buf, fname);
    char *b;
    for (b = buf; *b; b++) {
        if (isspace(*b) || (*b == '(')) {
            *b = '\0';
            break;
        }
    }
    if (Sp.CheckFuncName(buf)) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "%s is a predefined function.\n",
            buf);
        return (false);
    }

    // Parse the function body. We can't know if there are the right
    // number of undefined variables in the expression.  First remove
    // any quoting.
    //
    char *bstr = 0;
    if (*body == '\'' || *body == '"') {
        const char *t = body + strlen(body) - 1;
        if (*t == *body) {
            bstr = lstring::copy(body + 1);
            bstr[strlen(bstr) - 1] = 0;
        }
    }
    if (!bstr)
        bstr = lstring::copy(body);

    // Set up to add a dummy node for unresolved functions, these
    // will be resolved as UDFs at exec time.
    Sp.SetFlag(FT_DEFERFN, true);
    const char *btmp = bstr;
    pnode *pn = Sp.GetPnode(&btmp, false);
    Sp.SetFlag(FT_DEFERFN, false);
    delete [] bstr;
    if (!pn)
        return (false);

    // This is a pain -- when things are garbage-collected, any
    // vectors that may have been mentioned here will be thrown
    // away. So go down the tree and save any vectors.
    //
    pn->copyvecs();
    
    // Format the name properly and add to the list.

    // First remove all white space.
    char *namebf = new char[strlen(fname) + 1];
    b = namebf;
    for (const char *s = fname; *s; s++) {
        if (isspace(*s))
            continue;
        *b++ = *s;
    }
    *b = 0;

    // Now determine arity and separate arguments with null.
    int arity = 0;
    for (char *s = namebf; *s; s++) {
        if (*s == '(') {
            *s = '\0';
            if (s[1] != ')')
                arity++;    // It will have been 0
        }
        else if (*s == ')')
            *s = '\0';
        else if (*s == ',') {
            *s = '\0';
            arity++;
        }
    }

    strcpy(buf, namebf);
    char *s = strchr(buf, '(');
    if (s)
        *s = 0; 

    if (!ud_tab)
        ud_tab = new sHtab(sHtab::get_ciflag(CSE_UDF));

    sUdFunc *udf = (sUdFunc*)sHtab::get(ud_tab, buf);
    if (!udf)
        ud_tab->add(buf, new sUdFunc(namebf, arity, pn));
    else {
        sUdFunc *u = udf;
        for ( ; u; u = u->next()) {
            if (arity == u->argc()) {
                u->reuse(namebf, pn);
                break;
            }
        }
        if (u == 0) {
            u = new sUdFunc(namebf, arity, pn);
            u->set_next(udf->next());
            udf->set_next(u);
        }
    }
    CP.AddKeyword(CT_UDFUNCS, buf);
    return (true);
}


// Add a new entry, under a new unique name.  This is used to
// "promote" functions defined in the transient lists to the current
// cell list.
//
//  namep
//    Pointer the the function name address, the string at the address
//    will be updated if the name is changed (malloc'ed).  The name does
//    not include the args.
//
//  oldnames
//    The 0-separated name and argument list from the existing entry.
//
//  body
//    Function body text.
//
bool
cUdf::define_unique(const char **namep, const char *oldnames, const char *body)
{
    const char *ename = get_promoted(*namep, body);
    if (ename) {
        // Already have this.
        *namep = lstring::copy(ename);
        return (true);
    }

    // Save a pointer to the old name, change to new unique name.
    const char *oname = *namep;
    new_unique_name(namep);

    // Build an argument list.
    sLstr lstr;
    lstr.add(*namep);
    lstr.add_c('(');
    const char *t = oldnames;
    while (*t)
        t++;
    t++;
    while (*t) {
        lstr.add(t);
        while (*t)
            t++;
        t++;
        if (*t)
            lstr.add_c(',');
    }
    lstr.add_c(')');

    if (define(lstr.string(), body)) {
        set_promoted(*namep, oname, body);
        return (true);
    }
    return (false);
}


// Return true if the function name and arity, or name only if arity
// is negative, matches a saved definition.
//
//
bool
cUdf::is_defined(const char *name, int arity) const
{
    if (ud_tab) {
        sUdFunc *udf = (sUdFunc*)sHtab::get(ud_tab, name);
        for (sUdFunc *u = udf; u; u = u->next()) {
            if (arity < 0 || arity == u->argc())
                return (true);
        }
    }
    return (false);
}


// Return text and argument strings for the indicated function.
//
bool
cUdf::get_macro(const char *name, int arity, char** text,
    const char **args) const
{
    if (ud_tab) {
        sUdFunc *udf = (sUdFunc*)sHtab::get(ud_tab, name);
        for (sUdFunc *u = udf; u; u = u->next()) {
            if (arity < 0 || arity == u->argc()) {
                *text = u->tree()->get_string();
                const char *s = u->name();
                while (*s)
                    s++;
                s++;
                *args = s;
                return (true);
            }
        }
    }
    return (false);
}


// Search the list for a matching function definition, negative arity
// is a wildcard.
//
sUdFunc *
cUdf::find(const char *name, int arity) const
{
    if (ud_tab) {
        sUdFunc *udf = (sUdFunc*)sHtab::get(ud_tab, name);
        for (sUdFunc *u = udf; u; u = u->next()) {
            if (arity < 0 || u->argc() == arity)
                return (u);
        }
    }
    return (0);
}


// Return a copy of the parse tree for the indicated function, with
// args copied in.
//
pnode *
cUdf::get_tree(sUdFunc *udf, const pnode *args) const
{
    const char *t;
    for (t = udf->name(); *t; t++) ;
    t++;

    // Now we have to traverse the tree and copy it over, 
    // substituting args.
    //
    return (udf->tree()->copy(t, args));
}


// Clear the named definitions, or all definitions.
//
void
cUdf::clear(wordlist *wlist)
{
    if (!wlist)
        return;
    if (!ud_tab)
        return;
    if (*wlist->wl_word == '*' || lstring::cieq(wlist->wl_word, "all")) {
        sHgen gen(ud_tab, true);
        sHent *h;
        while ((h = gen.next()) != 0) {
            sUdFunc *un;
            for (sUdFunc *u = (sUdFunc*)h->data(); u; u = un) {
                un = u->next();
                delete u;
            }
            CP.RemKeyword(CT_UDFUNCS, h->name());
            delete h;
        }
        delete ud_tab;
        ud_tab = 0;
        return;
    }
    while (wlist) {
        sUdFunc *udf = (sUdFunc*)ud_tab->remove(wlist->wl_word);
        while (udf) {
            sUdFunc *ux = udf;
            udf = udf->next();
            delete ux;
        }
        CP.RemKeyword(CT_UDFUNCS, wlist->wl_word);
        wlist = wlist->wl_next;
    }
}


// Copy the whole mess, used for the subcircuit cache.
//
cUdf *
cUdf::copy() const
{
    cUdf *udfdb = new cUdf;
    if (ud_tab) {
        udfdb->ud_tab = new sHtab(sHtab::get_ciflag(CSE_UDF));

        sHgen gen(ud_tab);
        sHent *h;
        while ((h = gen.next()) != 0) {
            sUdFunc *udf = 0, *uend = 0;
            for (sUdFunc *u = (sUdFunc*)h->data(); u; u = u->next()) {
                if (!uend)
                    uend = udf = sUdFunc::copy(u);
                else {
                    uend->set_next(sUdFunc::copy(u));
                    uend = uend->next();
                }
            }
            if (udf)
                udfdb->ud_tab->add(h->name(), udf);
        }
    }
    return (udfdb);
}


// For debugging.
//
void
cUdf::print() const
{
    const cUdf *cudf = this;
    if (!cudf)
        return;

    sHgen gen(ud_tab);
    sHent *h;
    while ((h = gen.next()) != 0) {
        for (sUdFunc *u = (sUdFunc*)h->data(); u; u = u->next()) {
            char *s = u->tree()->get_string();
            printf("%s:  %s\n", u->name(), s);
            delete [] s;
        }
    }
}


#define UNIQUE_SEP '#'

// On return, namep points to a new, unique name (malloc'ed);
//
void
cUdf::new_unique_name(const char **namep)
{
    char buf[256];
    char *t = lstring::stpcpy(buf, *namep);
    *t++ = UNIQUE_SEP;
    for (int i = 1; ; i++) {
        snprintf(t, 6, "%d", i);
        if (!sHtab::get(ud_tab, buf))
            break;
    }
    *namep = lstring::copy(buf);
}


// Return the new name if oname/body have a promoted tab entry.
//
const char *
cUdf::get_promoted(const char *oname, const char *body)
{
    if (!ud_promo_tab)
        return (0);
    char *tmp = new char[strlen(oname) + strlen(body) + 1];
    char *t = lstring::stpcpy(tmp, oname);
    strcpy(t, body);
    const char *ret = (const char*)sHtab::get(ud_promo_tab, tmp);
    delete [] tmp;
    return (ret);
}


// Save an entry in the promoted table for oname/body pointing at the
// new name nname.
//
void
cUdf::set_promoted(const char *nname, const char *oname, const char *body)
{
    if (!ud_promo_tab)
        ud_promo_tab = new sHtab(sHtab::get_ciflag(CSE_UDF));
    char *tmp = new char[strlen(oname) + strlen(body) + 1];
    char *t = lstring::stpcpy(tmp, oname);
    strcpy(t, body);
    ud_promo_tab->add(tmp, lstring::copy(nname));
    delete [] tmp;
}
// End of cUdf functions.


sUdFunc::~sUdFunc()
{
    delete [] ud_name;
    delete ud_text;
}


// Static function.
sUdFunc *
sUdFunc::copy(const sUdFunc *ud)
{
    if (!ud)
        return (0);

    // In ud_name, the args are separated from the function name and
    // each other by nulls.  The token is terminated by two nulls.
    const char *t = ud->ud_name;
    while (*t)
        t++;
    for (int i = 0; i < ud->ud_arity; i++) {
        t++;
        while (*t)
            t++;
    }
    int len = t - ud->ud_name + 2;
    char *n = new char[len];
    memcpy(n, ud->ud_name, len);

    pnode *p = ud->ud_text->copy();
    return (new sUdFunc(n, ud->ud_arity, p));
}


void
sUdFunc::reuse(char *bf, pnode *pn)
{
    delete [] ud_name;
    ud_name = bf;
    delete ud_text;
    ud_text = pn;
}
// End of sUdFunc functions.


udf_text::udf_text(sUdFunc *ud)
{
    // Print the head
    char buf[BSIZE_SP];
    char *s = lstring::stpcpy(buf, ud->name());
    *s++ = ' ';
    *s++ = '(';
    const char *t = ud->name() + strlen(ud->name());
    t++;
    while (*t) {
        while (*t)
            *s++ = *t++;
        if (t[1]) {
            *s++ = ',';
            *s++ = ' ';
        }
        t++;
    }
    *s++ = ')';
    *s = 0;

    name = lstring::copy(buf);
    arity = ud->argc();
    text = ud->tree()->get_string();
    is_shell = false;
}

