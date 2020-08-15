
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

#ifdef WRSPICE
// Time profiling.
//#define TIME_DEBUG

#include "cshell.h"
#include "simulator.h"
#include "parser.h"
#include "input.h"
#include "inpline.h"
#include "ttyio.h"
#include "output.h"
#include "spnumber/paramsub.h"
#include "spnumber/spnumber.h"

#else
#include "main.h"
#include "sced.h"
#include "spnumber/paramsub.h"
#include "spnumber/spnumber.h"
struct ParseNode;
#include "spnumber/spparse.h"
#endif


char *sParamTab::errString = 0;
void(*sParamTab::pt_set_predefs)(sParamTab*);

namespace {
    char *ptok(char**, char**, char**);
    char *nametok(const char**, const char**, bool);
    char *valtok(const char**);

    // Valid chars for first char in parameter name.
    inline bool is_namechar(char c) { return (isalpha(c) || c == '_'); }
}


namespace {
    void free_param(void *pp, void*)
    {
        delete (sParam*)pp;
    }
}


sParamTab::~sParamTab()
{
    pt_table->clear_data(&free_param, 0);
    delete pt_table;
    delete pt_rctab;
    delete pt_ctrl;
}


// Static function.
//
sParamTab *
sParamTab::copy(const sParamTab *pt)
{
    if (!pt)
        return (0);

    sParamTab *pnew = new sParamTab;
    sHgen gen(pt->pt_table);
    sHent *h;
    while ((h = gen.next()) != 0) {
        sParam *po = (sParam*)h->data();
        sParam *p = new sParam(lstring::copy(po->name()), 0);
        p->update(po);
        pnew->pt_table->add(p->name(), p);
    }
    return (pnew);
}


namespace {
    // Count and return the comma-separated tokens in the argument
    // list string.
    //
    int count_args(const char *str)
    {
        // These are the non-alpha chars that we allow to start
        // an argument token.
        static const char *argchars = "_#@$";

        int n = 0;
        const char *s = str;
        while (*s == '(' || isspace(*s))
            s++;
        for (;;) {
            while (isspace(*s))
                s++;
            if (isalpha(*s) || strchr(argchars, *s))
                n++;
            else
                return (-1);  // syntax error
            while (*s && *s != ',' && *s != ')')
                s++;
            if (!*s || *s == ')')
                break;
            s++;
        }
        return (n);
    }


    // If *namep is a macro definition in the form name(arg,...),
    // return true and rewrite *namep as "name(argcnt)" and return a
    // new sArgList.
    //
    bool is_func(char **namep, sArgList **alp)
    {
        char *s, *pname = *namep;
        if (strchr(pname, '(') && (s = strrchr(pname, ')')) &&
                (!s[1] || isspace(s[1]))) {
            char *t = strchr(pname, '(');
            int ac = count_args(t);
            if (ac >= 0) {
                int n = s - t + 1;
                char *av = new char[n + 1];
                strncpy(av, t, n);
                av[n] =  0;
                *alp = new sArgList(av, ac);

                // The name token of the function is funcname(arg_cnt).
                char *pn = new char[t - pname + 6];
                t = pname;
                s = pn;
                while (*t && !isspace(*t) && *t != '(')
                    *s++ = *t++;
                *s++ = '(';
                sprintf(s, "%d)", ac);
                delete [] pname;
                *namep = pn;
                return (true);
            }
        }
        return (false);
    }
}


// Static function.
// Override any currently assigned params in 'this' list, or add if
// not found.  'this' can be null.  The str is a .param line.
//
sParamTab *
sParamTab::extract_params(sParamTab *thispt, const char *str)
{
    sParamTab *ptab = thispt;
    if (!str)
        return (ptab);

    // Strip leading SPICE keyword.
    if (*str == '.') {
        while (*str && !isspace(*str))
            str++;
        while (isspace(*str))
            str++;
    }

    while (*str) {
        char *pname, *psub;
        if (!tokenize(&str, &pname, &psub, PTparam))
            break;
        // Intercept ".param func(...) = xxxxx" function definitions here.
        sArgList *al;
        if (is_func(&pname, &al)) {
            sParam *p = 0;
            if (ptab) {
                p = (sParam*)sHtab::get(ptab->pt_table, pname);
                if (p) {
                    delete [] p->sub();
                    p->set_sub(psub);
                    p->set_arglist(al);
                    delete [] pname;
                }
            }
            if (!p) {
                p = new sParam(pname, psub);
                p->set_arglist(al);
                if (!ptab)
                    ptab = new sParamTab;
                ptab->pt_table->add(pname, p);
            }
        }
        else {
            sParam *p = 0;
            if (ptab) {
                p = (sParam*)sHtab::get(ptab->pt_table, pname);
                if (p) {
                    if (!p->readonly()) {
                        delete [] p->sub();
                        p->set_sub(psub);
                    }
                    else
                        delete [] psub;
                    delete [] pname;
                }
            }
            if (!p) {
                p = new sParam(pname, psub);
                if (!ptab)
                    ptab = new sParamTab;
                ptab->pt_table->add(pname, p);
            }
        }
    }
    return (ptab);
}


// Static function.
// Update the param values in this from the passed table.  If the override
// name is not found in this, add the assignment to this.
//
sParamTab *
sParamTab::update(sParamTab *thispt, const sParamTab *ptab)
{
    sParamTab *p0 = thispt;
    if (!ptab)
        return (p0);
    if (!p0)
        p0 = new sParamTab;

    sHgen gen(ptab->pt_table);
    sHent *ent;
    while ((ent = gen.next()) != 0) {
        const sParam *p = (const sParam*)ent->data();
        if (!p)
            // impossible
            continue;
        sParam *q = (sParam*)sHtab::get(p0->pt_table, p->name());
        if (q) {
            if (!q->readonly())
                q->update(p);
        }
        else {
            sParam *pnew = new sParam(lstring::copy(p->name()), 0);
            pnew->update(p);
            p0->pt_table->add(pnew->name(), pnew);
        }
    }
    return (p0);
}


// Override assigned parameters from those in str, if override name is
// not found in 'this' list, add the assignment.
//
void
sParamTab::update(const char *str)
{
    if (!str)
        return;
    while (*str) {
        char *pname, *psub;
        if (!tokenize(&str, &pname, &psub, PTparam))
            break;
        sArgList *al;
        if (is_func(&pname, &al)) {
            sParam *p = (sParam*)sHtab::get(pt_table, pname);
            if (p && !p->readonly()) {
                delete [] p->sub();
                p->set_sub(psub);
                p->set_arglist(al);
            }
            else {
                delete [] psub;
                delete al;
            }
            delete [] pname;
        }
        else {
            sParam *p = (sParam*)sHtab::get(pt_table, pname);
            if (!p) {
                p = new sParam(pname, psub);
                pt_table->add(p->name(), p);
            }
            else {
                if (p && !p->readonly()) {
                    // Expand!  Needed for "param=param" recursion.
                    line_subst(&psub);
                    if (psub != p->sub()) {
                        delete [] p->sub();
                        p->set_sub(psub);
                    }
                }
                else
                    delete [] psub;
                delete [] pname;
            }
        }
    }
}


// Evaluate numerically the expanded substitution string.
//
double
sParamTab::eval(const sParam *p, bool *err) const
{
    if (p) {
        char *expr = lstring::copy(p->sub());
        line_subst(&expr);
        const char *eptr = expr;
        if (err)
            *err = false;
#ifdef WRSPICE
        pnode *pn = Sp.GetPnode(&eptr, true, true, true);
        delete [] expr;
        sDataVec *dv = 0;
        if (pn) {
            dv = Sp.Evaluate(pn);
            delete pn;
        }
        if (dv)
            return (dv->realval(0));
        else if (err)
            *err = true;
#else
        double *dp = SCD()->evalExpr(&eptr);
        if (dp)
            return (*dp);
        else if (err)
            *err = true;
#endif
    }
    return (0.0);
}


// Look through the table, and add macros (user-defined functions) to
// the UDF database.  Generally, we push to a fresh UDF context first. 
// If nopush is true, this is skipped, and the functions will be added
// to whatever context is currently active.
//
// This must be called before macros are evaluated.
//
void
sParamTab::define_macros(bool nopush)
{
#ifdef WRSPICE
    if (!nopush)
        Sp.PushUserFuncs(0);

    sHgen gen(pt_table);
    sHent *ent;
    while ((ent = gen.next()) != 0) {
        sParam *p = (sParam*)ent->data();
        if (p->numargs() >= 0) {
            // p->name() has form "name(argc)"
            char *call = new char[strlen(p->name()) + strlen(p->args())];
            strcpy(call, p->name());
            char *t = strchr(call, '(');
            if (t)
                // always true
                *t = 0;
            // The args string contains the parentheses.
            strcat(call, p->args());
            Sp.DefineUserFunc(call, p->sub());
            delete [] call;
        }
    }
#else
    (void)nopush;
#endif
}


// This should only be called after define_macros(false).  It removes
// and destroys the macros saved.
//
void
sParamTab::undefine_macros()
{
#ifdef WRSPICE
    delete Sp.PopUserFuncs();
#endif
}


// Debugging: dump all entries to stdout.
//
void
sParamTab::dump() const
{
    sHgen gen(pt_table);
    sHent *ent;
    while ((ent = gen.next()) != 0) {
        sParam *po = (sParam*)ent->data();
        printf("%-16s  %s\n", ent->name(), po->sub());
        if (strcmp(ent->name(), po->name()))
            printf("BAD %s %s\n", ent->name(), po->name());
    }
}


namespace {
    // Traditional SPICE delimeters.
    inline bool issep(int c)
    {
        return (isspace(c) || c == '=' || c == '(' || c == ')' || c == ',');
    }
}


// Static function.
// Update a line containing param=value constructs.  The RHS of each
// definition is updated.  This is applied to .model, .param, .subckt,
// and X lines (according to mode).
//
// PTgeneral:  Ignore bad definition constructs, these might not really
//             be parameter definitions.
// PTsngl:     Accept p=v and isolated parms and expressions.
// PTparam:    Fail on bad definition construct.
// PTsubc:     The tricky case is for .subckt lines.  In this case, we
//             copy 'this' and add/update the parameter definitions as
//             found.  This is needed to support use of parameters defined
//             to the left, e.g., in sequences like "p1=1 p2=2 p3="p1+p2'".
//
void
sParamTab::defn_subst(const sParamTab *thispt, char **str, PTmode mode,
    int nskip)
{
    if (!str || !*str)
        return;

    const sParamTab *ptab = thispt;
    if (!ptab && mode != PTsubc)
        return;
    sParamTab *tmp_tab = 0;
    if (mode == PTsubc) {
        tmp_tab = new sParamTab;
        tmp_tab = update(tmp_tab, ptab);
        ptab = tmp_tab;
    }

    // Skip leading white space.
    const char *s0 = *str;
    while (isspace(*s0))
        s0++;

    // Skip nskip tokens.
    while (*s0 && nskip > 0) {
        while (*s0 && !issep(*s0))
            s0++;
        while (issep(*s0))
            s0++;
        nskip--;
    }

    while (*s0) {
        const char *start;
        char *pname, *psub;
        if (!ptab->tokenize(&s0, &pname, &psub, mode, &start))
            break;
        if (mode == PTsngl && !psub) {
            psub = pname;
            pname = 0;
            // only case where pname is 0
        }

        if (pname) {
            char *s;
            if (strchr(pname, '(') && (s = strrchr(pname, ')')) &&
                    (!s[1] || isspace(s[1]))) {
                // LHS is a function definition, skip this.
                delete [] pname;
                delete [] psub;
                continue;
            }
        }

        char *tsub = lstring::copy(psub);
        ptab->line_subst(&psub);

        if (mode == PTsubc) {
            // We're parsing a .subckt line.  'this' is a COPY of the
            // parent context, and we add the parameters as we go
            // along.  This is so constructs like "x=1 y=1 z='x+y'"
            // will work.

            sParam *p = (sParam*)sHtab::get(tmp_tab->pt_table, pname);
            if (p) {
                if (!p->readonly()) {
                    delete [] p->sub();
                    p->set_sub(lstring::copy(psub));
                }
            }
            else {
                p = new sParam(lstring::copy(pname), lstring::copy(psub));
                tmp_tab->pt_table->add(pname, p);
            }
        }

        if (!strcmp(tsub, psub)) {
            delete [] pname;
            delete [] psub;
            delete [] tsub;
            continue;
        }
        delete [] tsub;

        int len = (start - *str) + strlen(psub) + strlen(s0) + 10;
        if (pname)
            len += strlen(pname);
        char *nstr = new char[len];
        strncpy(nstr, *str, start - *str);
        char *e = lstring::stpcpy(nstr + (start - *str), pname ? pname : psub);
        if (pname) {
            *e++ = '=';
            e = lstring::stpcpy(e, psub);
        }
        if (*s0) {
            *e++ = ' ';
            strcpy(e, s0);
        }
        delete [] pname;
        delete [] psub;

        s0 = e;
        delete [] *str;
        *str = nstr;
    }

    if (mode == PTsubc)
        delete tmp_tab;
}


// Parameter and single-quote expand str.  If a substitution is done,
// *str is replaced.  Error messages go to sParamTab::errString.
//
void
sParamTab::line_subst(char **str) const
{
    if (!str || !*str)
        return;

    char *in = *str, *tok, *start = 0, *end = 0;
    while ((tok = ptok(&in, &start, &end)) != 0) {

        bool chg = false;
        if (*tok == '\'') {
            squote_subst(&tok);
            chg = true;
        }
        else if (is_namechar(*tok)) {
            char *ltok = lstring::copy(tok);
            if (subst(&tok)) {
                pt_rctab->add(ltok, (void*)1);
                if (*tok == '\'')
                    squote_subst(&tok);
                else {
                    if (sHtab::get(pt_rctab, tok)) {
                        // Uh-oh, we're already subbing this token,
                        // there is a recursive loop.
                        delete [] errString;
                        sLstr lstr;
                        lstr.add("Recursion detected, parameter name: ");
                        lstr.add(ltok);
                        lstr.add(" value: ");
                        lstr.add(tok);
                        errString = lstr.string_trim();
                    }
                    else
                        line_subst(&tok);
                }

                pt_rctab->remove(ltok);
                chg = true;
            }
            delete [] ltok;
        }
        if (chg) {
            if (start > *str && start[-1] == '%')
                // Swallow leading concatenation character.  The '%'
                // character can be used to separate a parameter from
                // trailing characters.
                start--;
            char *nstr = new char[start - *str + strlen(tok)
                + strlen(end) + 1];
            strncpy(nstr, *str, (start - *str));
            char *s = nstr + (start - *str);
            strcpy(s, tok);
            while (*s)
                s++;
            in = s;
            if (*end == '%')
                // Swallow trailing concatenation character.
                end++;
            strcpy(s, end);
            delete [] *str;
            *str = nstr;
        }
        delete [] tok;
    }
}


// Parameter expand and perhaps evaluate the single-quoted expression. 
// If evaluation succeeds, the expression is freed and replaced with
// result.  If the expression contains circuit references or
// unexpaanded shell variables, parameter expand as much as possible
// and keep str as a single-quoted expression.
//
void
sParamTab::squote_subst(char **str) const
{
    if (pt_ctrl->no_sqexp || strchr(*str, '$')) {
        // We aren't single-quote expanding, or The expression
        // contains unexpanded shell variables.  In this case expand
        // any parameters, and leave the result in single quotes.

        char *expr;
        bool quoted = false;
        if (**str == '\'') {
            // strip quotes;
            expr = lstring::copy(*str + 1);
            char *t = expr + strlen(expr) - 1;
            if (*t == '\'')
                *t = 0;
            quoted = true;
        }
        else
            expr = lstring::copy(*str);
        line_subst(&expr);
        if (quoted) {
            char *s = new char[strlen(expr) + 3];
            *s = '\'';
            strcpy(s+1, expr);
            strcat(s, "\'");
            delete [] expr;
            expr = s;
        }
        delete [] *str;
        *str = expr;
        return;
    }

    evaluate(str);
}


// If tok is a parameter, perform the substitution and return true.
//
bool
sParamTab::subst(char **tok) const
{
    // Special case, parameter definition check:  def(foo) is replaced
    // by 1/0 if foo is defined/undefined.
    char *t = *tok;
    if (lstring::ciprefix("def(", t)) {
        t += 4;
        int n = strlen(t) - 1;
        if (t[n] == ')')
            t[n] = 0;
        sParam *p = (sParam*)get(t);
        if (p)
            (*tok)[0] = '1';
        else
            (*tok)[0] = '0';
        (*tok)[1] = 0;
        return (true);
    }

    sParam *p = (sParam*)get(*tok);
    if (p) {
        if (sHtab::get(pt_rctab, *tok) != 0) {
            // Uh-oh, we're already subbing this token, there is a
            // recursive loop.
            delete [] errString;
            sLstr lstr;
            lstr.add("Recursion detected, parameter name: ");
            lstr.add(p->name());
            lstr.add(" value: ");
            lstr.add(p->sub());
            errString = lstr.string_trim();
            return (false);
        }
        pt_rctab->add(p->name(), p);

        if (pt_ctrl->collapse) {
            if (!p->collapsed())
                p->collapse(this);
        }

        delete [] *tok;
        if (*p->sub() == '"') {
            // strip quotes
            *tok = lstring::copy(p->sub()+1);
            char *tt = *tok + strlen(*tok) - 1;
            if (*tt == '"')
                *tt = 0;
        }
        else {
            *tok = lstring::copy(p->sub());
//XXX NEW
            evaluate(tok);
        }

        pt_rctab->remove(p->name());
        return (true);
    }
    return (false);
}


void
sParamTab::evaluate(char **str) const
{
    const char *msg = "Evaluation failed: %s.";
    char *expr;

#ifdef WRSPICE
    bool quoted = false;
#endif
    if (**str == '\'') {
        // strip quotes;
        expr = lstring::copy(*str + 1);
        char *t = expr + strlen(expr) - 1;
        if (*t == '\'')
            *t = 0;
#ifdef WRSPICE
        quoted = true;
#endif
    }
    else
        expr = lstring::copy(*str);

    // Have to clean out the temp vectors here, otherwise memory used
    // by stale vectors can build up to unacceptable levels.

#ifdef TIME_DEBUG
    double tstart = OP.seconds();
#endif
    line_subst(&expr);
#ifdef WRSPICE
    sDataVec::set_temporary(true);
#endif
#ifdef TIME_DEBUG
    double tend = OP.seconds();
    //printf("sq1: %g\n", tend - tstart);
    tstart = tend;
#endif

#ifdef WRSPICE
    // Parse the expression but don't check tree, we'll check it here.
    const char *eptr = expr;
    pnode *pn = Sp.GetPnode(&eptr, false, true);
    int tc = 0;
    if (!pn || (tc = pn->checktree()) == 2) {
        // Major parse error or empty expression.
        char *er = new char[strlen(msg) + strlen(*str) + 10];
        sprintf(er, msg, *str);
        delete [] errString;
        errString = er;
        sDataVec::set_temporary(false);
        delete [] expr;
        delete pn;
        return;
    }
    if (tc == 1) {
#ifdef TIME_DEBUG
        tend = OP.seconds();
        printf("sq2: %g\n", tend - tstart);
        tstart = tend;
#endif
        // Tree contains v(...) references, can't evaluate.  Leave
        // it as-is: single-quoted, with parameters expanded as much
        // as possible.

        // Any transient UDFs are saved in the current cell UDF table
        // under a new name, and the parse nodes will point to these
        // new functions. 
        pn->promote_macros(this);
#ifdef TIME_DEBUG
        tend = OP.seconds();
        printf("sq3: %g\n", tend - tstart);
        tstart = tend;
#endif

        // Recreate the text, which has updated macro names.
        delete [] expr;
        expr = pn->get_string(quoted);

        delete [] *str;
        *str = expr;
        sDataVec::set_temporary(false);
        delete pn;
#ifdef TIME_DEBUG
        tend = OP.seconds();
        printf("sq4: %g\n", tend - tstart);
        tstart = tend;
#endif
        return;
    }

    sDataVec *dv = Sp.Evaluate(pn);
    delete pn;
    if (!dv) {
        char *er = new char[strlen(msg) + strlen(*str) + 10];
        sprintf(er, msg, *str);
        delete [] errString;
        errString = er;
        sDataVec::set_temporary(false);
        delete [] expr;
        return;
    }
    sDataVec::set_temporary(false);
    delete [] expr;
    delete [] *str;
    *str = lstring::copy(SPnum.printnum(dv->realval(0), dv->units(), false));
    OP.vecGc(true);
#else  // WRSPICE
    const char *eptr = expr;
    double *dp = SCD()->evalExpr(&eptr);
    if (!dp) {
        char *er = new char[strlen(msg) + strlen(*str) + 10];
        sprintf(er, msg, *str);
        delete [] errString;
        errString = er;
        delete [] expr;
        return;
    }
    delete [] expr;
    delete [] *str;
    *str = lstring::copy(SPnum.printnum(*dp));
#endif  // WRSPICE
}


// Static function.
// Grab the param = value and advance pstr.  If mode == PTparam, fail
// on a bad construct, otherwise silently skip over bad constructs. 
// If mode == PTsngl, return isolated (i.e., without following value)
// 'names' with psub pointing at null.
//
bool
sParamTab::tokenize(const char **pstr, char **pname, char **psub,
    PTmode mode, const char **pstart)
{
    *pname = 0;
    *psub = 0;

    char *name = 0;
    while (*pstr) {
        name = nametok(pstr, pstart, (mode == PTparam));
        if (!name)
            return (false);
        if (mode == PTparam) {
            if (*name == '$')
                // This is either the start of an HSPICE-type comment, or a
                // misplaced shell expansion.  Try to sync up with the next
                // viable parameter definition.
                mode = PTgeneral;
            else {
                if (!is_namechar(*name)) {
                    sLstr lstr;
                    lstr.add("Bad parameter name: ");
                    lstr.add(name);
                    lstr.add_c('.');
                    delete [] name;
                    delete [] errString;
                    errString = lstr.string_trim();
                    return (false);
                }
                if (**pstr != '=') {
                    const char *msg = "Parameter syntax error, misplaced '='.";
                    delete [] errString;
                    errString = lstring::copy(msg);
                    delete [] name;
                    return (false);
                }
            }
        }
        if (**pstr == '=')
            break;
        if (mode == PTsngl && (is_namechar(*name) || *name == '\'')) {
            // Found an isolated name or expression, return it.
            *pname = name;
            *psub = 0;
            return (true);
        }
        delete [] name;
    }
    if (**pstr != '=') {
        delete [] name;
        return (false);
    }
    if (!is_namechar(*name)) {
        sLstr lstr;
        lstr.add("Bad parameter name: ");
        lstr.add(name);
        lstr.add_c('.');
        delete [] name;
        delete [] errString;
        errString = lstr.string_trim();
        return (false);
    }
    while (isspace(**pstr) || **pstr == '=')
        (*pstr)++;

    if (!**pstr) {
        sLstr lstr;
        lstr.add("Missing parameter value for ");
        lstr.add(name);
        delete [] errString;
        errString = lstr.string_trim();
        delete [] name;
        return (false);
    }
    char *sub = valtok(pstr);

    const char *t = *pstr;
    while (isspace(*t) || *t == ',')
        t++;
    *pstr = t;

    *pname = name;
    *psub = sub;
    return (true);
}
// End of sParamTab functions


namespace {
    char *ptok(char **s, char **strt, char **end)
    {
        const char *tchars = Parser::default_specials();

        sLstr lstr;
        if (s == 0 || *s == 0)
            return (0);
        char *str = *s;
        while (isspace(*str) || lstring::instr(tchars, *str))
            str++;
        if (!*str) {
            *s = str;
            return (0);
        }
        *strt = str;
        if (*str == '\'' || *str == '"') {
            char c = *str++;
            lstr.add_c(c);
            while (*str) {
                lstr.add_c(*str);
                if (*str == c && *(str-1) != '\\') {
                    str++;
                    break;
                }
                str++;
            }
        }
        else {
            while (*str && !(isspace(*str) || lstring::instr(tchars, *str)))
                lstr.add_c(*str++);
        }

        // Special case, return "def(foo)" as one token.  White space
        // can appear around the parens, but is stripped in returned
        // token.
        //
        if (lstring::cieq(lstr.string(), "def")) {
            char *s0 = str;
            bool found = false;
            while (isspace(*str))
                str++;
            if (*str == '(') {
                str++;
                while (isspace(*str))
                    str++;
                while (*str && !lstring::instr(tchars, *str))
                    str++;
                while (isspace(*str))
                    str++;
                if (*str == ')') {
                    str++;
                    lstr.add_c('(');
                    while (s0 < str) {
                        if (*s0 != '(' && *s0 != ')' && !isspace(*s0))
                            lstr.add_c(*s0);
                        s0++;
                    }
                    found = true;
                }
            }
            if (!found)
                str = s0;
        }
        *end = str;

        while (isspace(*str) || lstring::instr(tchars, *str))
            str++;
        *s = str;
        return (lstr.string_trim());
    }


    // Return the parameter name token, and advance s.  If lhs_funcs
    // is true, the extracted name can be in the form "xxxx(...)",
    // i.e., a function call.  Otherwise, parentheses are treated as
    // delimeters.
    //
    // Functions are extracted as LHS tokens in .param lines.  These
    // are "user defined" function specifications.
    //
    char *nametok(const char **s, const char **pstart, bool lhs_funcs)
    {
        if (s == 0 || *s == 0)
            return (0);
        const char *delim = lhs_funcs ? "," : ",()";
        const char *str = *s;
        while (isspace(*str) || (*str && strchr(delim, *str)))
            str++;
        if (pstart)
            *pstart = str;
        if (!*str) {
            *s = str;
            return (0);
        }

        const char *start = str;
        if (*str == '\'' || *str == '"') {
            // Quoted.  Keep the quote marks, the quoted quantity is
            // the value.

            char c = *str++;
            while (*str) {
                if (*str == c && *(str-1) != '\\')
                    break;
                str++;
            }
            if (*str)
                str++;
        }
        else {
            int pn = 0;
            if (lhs_funcs) {
                while (*str) {
                    if (!pn) {
                        if (isspace(*str)) {
                            // Check if the next char is '(', if so
                            // continue.

                            const char *t = str;
                            while (isspace(*t))
                                t++;
                            if (*t == '(') {
                                str = t;
                                continue;
                            }
                            break;
                        }
                        if (*str == '=')
                            break;
                    }
                    if (*str == '(')
                        pn++;
                    else if (*str == ')')
                        pn--;
                    str++;
                }
            }
            else {
                while (*str) {
                    if (isspace(*str) || *str == '=' || strchr(delim, *str))
                        break;
                    str++;
                }
            }
        }

        char *name = new char[str - start + 1];
        char *n = name;
        for (const char *t = start; t < str; t++) {
            if (!lhs_funcs || !isspace(*t))
                *n++ = *t;
        }
        *n = 0;

        while (isspace(*str) || (*str && strchr(delim, *str)))
            str++;
        *s = str;
        return (name);
    }


    char *valtok(const char **s)
    {
        const char *str = *s;
        while (isspace(*str))
            str++;
        if (!*str) {
            *s = str;
            return (0);
        }
        const char *start = str;
        bool keepws = false;
        if (*str == '\'' || *str == '"') {
            // Quoted.  Keep the quote marks, the quoted quantity is
            // the value.

            keepws = true;
            char c = *str++;
            while (*str) {
                if (*str == c && *(str-1) != '\\')
                    break;
                str++;
            }
            if (*str)
                str++;
        }
        else {
            // This can be a function call, with white space in the
            // argument list.

            int pn = 0;
            while (*str) {
                if (!pn) {
                    if (isspace(*str)) {
                        // Check if the next char is '(', if so
                        // continue.

                        const char *t = str;
                        while (isspace(*t))
                            t++;
                        if (*t == '(') {
                            str = t;
                            continue;
                        }
                        break;
                    }
                    else if (*str == ',')
                        break;
                }
                if (*str == '(')
                    pn++;
                else if (*str == ')') {
                    if (!pn)
                        break;
                    pn--;
                }
                str++;
            }
        }

        char *val = new char[str - start + 1];
        char *n = val;
        for (const char *t = start; t < str; t++) {
            if (keepws || !isspace(*t))
                *n++ = *t;
        }
        *n = 0;
        *s = str;
        return (val);
    }
}

