
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

#include "cd.h"
#include <algorithm>


sVbak::sVbak(const char *name, sVbak *vnext)
{
    vb_name = name;
    vb_value = name ? lstring::copy(CDvdb()->getVariable(name)) : 0;
    vb_func = 0;
    next = vnext;
}


sVbak::~sVbak()
{
    delete [] vb_value;
}
// End of sVdb functions.


cCDvdb *cCDvdb::instancePtr = 0;

cCDvdb::cCDvdb()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cCDvdb already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    v_tab = 0;
    v_tab_bak = 0;
    v_regvar_tab = 0;
    v_global_set_hook = 0;
    v_global_get_hook = 0;
    v_post_func = 0;
}


cCDvdb::~cCDvdb()
{
    v_tab->clear();
    delete v_tab;
    v_tab_bak->clear();
    delete v_tab_bak;
    v_regvar_tab->clear();
    delete v_regvar_tab;
}


// Private static error exit.
//
void
cCDvdb::on_null_ptr()
{
    fprintf(stderr, "Singleton class cCDvdb used before instantiated.\n");
    exit(1);
}


// Set the variable in name to string.  False is returned if setting
// an internal variable fails, otherwise true is returned.
//
bool
cCDvdb::setVariable(const char *vname, const char *string)
{
    v_post_func = 0;
    char *name = lstring::gettok(&vname);
    if (!name)
        return (true);

    char *expstr = expand(string, true);
    string = expstr;

    if (v_global_set_hook && (*v_global_set_hook)(name, string)) {
        delete [] expstr;
        delete [] name;
        return (true);
    }
    if (!string)
        string = "";
    while (isspace(*string))
        string++;
    if (v_regvar_tab) {
        intvar_t<CDvarProc> *iv = v_regvar_tab->find(name);
        if (iv && iv->cb) {
            // A false return from the callback inhibits setting.
            if (!(*iv->cb)(string, true)) {
                delete [] expstr;
                delete [] name;
                return (false);
            }
        }
    }
    if (!v_tab)
        v_tab = new table_t<var_t>;
    var_t *v = v_tab->find(name);
    if (!v) {
        v = new var_t(name, lstring::copy(string));
        v_tab->link(v, false);
        v_tab = v_tab->check_rehash();
    }
    else {
        delete [] name;
        char *nstr = lstring::copy(string);
        delete [] v->string;
        v->string = nstr;
    }
    update();
    if (v_post_func) {
        (*v_post_func)(v->string);
        v_post_func = 0;
    }
    delete [] expstr;
    return (true);
}


// Return the string for the variable named in name, null if not found.
//
const char *
cCDvdb::getVariable(const char *vname)
{
    char *name = lstring::gettok(&vname);
    if (!name)
        return (0);
    if (v_global_get_hook) {
        const char *tstr = (*v_global_get_hook)(name);
        if (tstr) {
            delete [] name;
            return (tstr);
        }
    }
    if (!v_tab)
        return (0);
    var_t *v = v_tab->find(name);
    delete [] name;
    if (!v)
        return (0);
    return (v->string);
}


// Clear (unset) the variable named in name.
//
void
cCDvdb::clearVariable(const char *vname)
{
    v_post_func = 0;
    if (!v_tab)
        return;
    char *name = lstring::gettok(&vname);
    if (!name)
        return;
    var_t *v = v_tab->find(name);
    if (v) {
        if (v_regvar_tab) {
            intvar_t<CDvarProc> *iv = v_regvar_tab->find(name);
            if (iv && iv->cb) {
                // A false return from the callback inhibits unsetting.
                if (!(*iv->cb)(v->string, false))
                    return;
            }
        }
        v_tab->unlink(v);
        delete v;
        update();
    }
    if (v_post_func) {
        (*v_post_func)(0);
        v_post_func = 0;
    }
    delete [] name;
}


// Static inline function.
// Sorting function for cCDvdb::list_vars()
//
inline bool
cCDvdb::vcomp(const cCDvdb::var_t *v1, const cCDvdb::var_t *v2)
{
    return (strcmp(v1->tab_name(), v2->tab_name()) < 0);
}


// Return a stringlist of the variables currently set, each string is
// name(space)value_if_any.
//
stringlist *
cCDvdb::listVariables()
{
    int numvars = 0;
    var_t **vars = 0;
    if (v_tab && v_tab->allocated() > 0) {
        numvars = v_tab->allocated();
        vars = new var_t*[numvars];
        tgen_t<var_t> gen(v_tab);
        var_t *v;
        int i = 0;
        while ((v = gen.next()) != 0)
            vars[i++] = v;
        if (numvars > 1)
            std::sort(vars, vars + numvars, vcomp);
    }

    stringlist *s0 = 0, *se = 0;
    for (int i = 0; i < numvars; i++) {
        const char *name = vars[i]->tab_name();
        const char *string = vars[i]->string;
        if (v_global_get_hook) {
            const char *tstr = (*v_global_get_hook)(vars[i]->tab_name());
            if (tstr)
                string = tstr;
        }

        int len = strlen(name);
        if (string && *string)
            len += strlen(string) + 1;
        len++;
        char *nstr = new char[len];
        char *t = lstring::stpcpy(nstr, name);
        if (string && *string) {
            *t++ = ' ';
            strcpy(t, string);
        }
        if (s0) {
            se->next = new stringlist(nstr, 0);
            se = se->next;
        }
        else
            se = s0 = new stringlist(nstr, 0);
    }
    delete [] vars;
    return (s0);
}


void
cCDvdb::update()
{
    CD()->ifVariableChange();
}


namespace {
    // If str is of the form "(...)" strip the parens and leading/trailing
    // white space and set hadp.
    //
    char *
    strip_parens(const char *str, bool *hadp)
    {
        const char *ep = str + strlen(str) - 1;
        if (*str == '(' && *ep == ')') {
            str++;
            while (isspace(*str))
                str++;
            char *ostr = lstring::copy(str);
            char *s = ostr + strlen(ostr) - 1;
            *s-- = 0;
            while (isspace(*s) && s >= str)
                *s-- = 0;
            *hadp = true;
            return (ostr);
        }
        return (lstring::copy(str));
    }
}


// Substitute in the string for any tokens of the form $(setvar).
// These are previously set variables, or environment variables, too,
// if use_env is true.
//
// If any of the substitutions are of the form "(...)", add
// surrounding parentheses to the returned string, if not already
// there, and strip parenthesis in substitution.  This is so that "set
// path = dir $(path)" will do the right thing.
//
char *
cCDvdb::expand(const char *string, bool use_env)
{
    if (!string)
        return (0);
    if (!*string)
        return (lstring::copy(""));
    sLstr lstr;
    const char *s = string;
    bool hadp = false;
    while (*s) {
        if (*s == '$' && *(s+1) == '(') {
            const char *t1 = s+2;
            char tok[128];
            char *t2 = tok;
            while (*t1 && *t1 != ')' && t1 - s < 128)
                *t2++ = *t1++;
            *t2 = 0;
            if (*t1 == ')') {
                const char *str = getVariable(tok);
                if (!str && (use_env))
                    str = getenv(tok);
                if (str) {
                    char *nstr = strip_parens(str, &hadp);
                    lstr.add(nstr);
                    delete [] nstr;
                    s = t1+1;
                    continue;
                }
            }
        }
        lstr.add_c(*s++);
    }
    char *t = lstr.string_trim();
    if (!hadp)
        return (t);
    char *e = t + strlen(t) - 1;
    if (*t == '(' && *e == ')')
        return (t);
    lstr.clear();
    if (*t != '(')
        lstr.add("( ");
    lstr.add(t);
    if (*e != ')')
        lstr.add(" )");
    delete [] t;
    return (lstr.string_trim());
}


// Copy the currently set variables into a backup table, which is
// returned.  The return type is opaque to the application.
//
CDvarTab *
cCDvdb::createBackup() const
{
    if (!v_tab)
        return (0);

    table_t<var_t> *tab_bak = new table_t<var_t>;
    tgen_t<var_t> gen(v_tab);
    var_t *v;
    while ((v = gen.next()) != 0) {
        var_t *vn = new var_t(
            lstring::copy(v->tab_name()), lstring::copy(v->string));
        tab_bak->link(vn);
        tab_bak = tab_bak->check_rehash();
    }
    return (tab_bak);
}


namespace {
    inline bool differ(const char *s, const char *t)
    {
        if (!s)
            s = "";
        if (!t)
            t = "";
        return (strcmp(s, t));
    }
}


// Revert the main table to the state of the backup.  The variables
// are reverted by calling setVariable and clearVariable so that the
// callbacks will be executed.  The backup table is not touched.
//
void
cCDvdb::revertToBackup(const CDvarTab *tab)
{
    table_t<var_t> *tab_bak = (table_t<var_t>*)tab;
    if (v_tab) {
        tgen_t<var_t> gen(v_tab);
        var_t *v;
        while ((v = gen.next()) != 0) {
            if (!tab_bak->find(v->tab_name()))
                clearVariable(v->tab_name());
        }
    }
    if (tab_bak) {
        tgen_t<var_t> gen(tab_bak);
        var_t *v;
        while ((v = gen.next()) != 0) {
            var_t *vv = v_tab->find(v->tab_name());
            if (!vv  || differ(v->string, vv->string))
                setVariable(v->tab_name(), v->string ? v->string : "");
        }
    }
}


// Destroy the backup table passed.
//
void
cCDvdb::destroyTab(CDvarTab *tab)
{
    table_t<var_t> *tab_bak = (table_t<var_t>*)tab;
    tgen_t<var_t> gen(tab_bak);
    var_t *v;
    while ((v = gen.next()) != 0) {
        tab_bak->unlink(v);
        delete v;
    }
    delete tab_bak;
}


// Register an internal variable and callback.  Return the previous
// callback function address, if any.
//
CDvarProc
cCDvdb::registerInternal(const char *vname, CDvarProc func)
{
    char *name = lstring::gettok(&vname);
    if (!name)
        return (0);
    if (!v_regvar_tab)
        v_regvar_tab = new table_t<intvar_t<CDvarProc> >;
    intvar_t<CDvarProc> *iv = v_regvar_tab->find(name);
    if (!iv) {
        iv = new intvar_t<CDvarProc>(name, func);
        v_regvar_tab->link(iv, false);
        v_regvar_tab = v_regvar_tab->check_rehash();
        return (0);
    }
    delete [] name;
    CDvarProc prev_func = iv->cb;
    iv->cb = func;
    return (prev_func);
}


// Like register_internal, but return a list element containing the
// previous state.
//
sVbak *
cCDvdb::pushInternal(sVbak *vb0, const char *vname, CDvarProc func)
{
    sVbak *vb = new sVbak(vname, vb0);
    vb->vb_func = registerInternal(vname, func);
    return (vb);
}


// Restore the variables to their original vakue and callback.  The
// passed list is destroyed.
//
void
cCDvdb::restoreInternal(sVbak *vbak)
{
    while (vbak) {
        if (vbak->vb_value)
            setVariable(vbak->vb_name, vbak->vb_value);
        else
            clearVariable(vbak->vb_name);
        registerInternal(vbak->vb_name, vbak->vb_func);
        sVbak *vx = vbak;
        vbak = vbak->next;
        delete vx;
    }
}


// Return a sorted list of registered variable names.
//
stringlist *
cCDvdb::listInternal()
{
    stringlist *s0 = 0;
    tgen_t<intvar_t<CDvarProc> > tgen(v_regvar_tab);
    intvar_t<CDvarProc> *iv;
    while ((iv = tgen.next()) != 0)
        s0 = new stringlist(lstring::copy(iv->tab_name()), s0);
    stringlist::destroy(s0);
    return (s0);
}

