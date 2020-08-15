
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

#ifndef PARAM_H
#define PARAM_H

#include "hash.h"


// Save the arg count and arguments of a function definition.
//
struct sArgList
{
    sArgList(char *av, int ac)
        {
            a_args = av;
            a_argc = ac;
        }

    ~sArgList()
        {
            delete [] a_args;
        }

    static sArgList *copy(const sArgList *a)
        {
            if (!a)
                return (0);
            return (new sArgList(lstring::copy(a->a_args), a->a_argc));
        }

    char *a_args;
    int a_argc;
};

class cUdf;

// Substitution parameter description
//
struct sParam
{
    sParam(char *n, char *s)
        {
            p_name = n;
            p_sub = s;
            p_args = 0;
            p_collapsed = false;
            p_readonly = false;
        }

    ~sParam()
        {
            delete [] name();
            delete [] p_sub;
            delete p_args;
        }

    const char *name()    const { return (p_name); }
    const char *sub()     const { return (p_sub); }
    void set_sub(char *s)       { p_sub = s; }

    const char *args()    const { return (p_args ? p_args->a_args : 0); }
    int numargs()         const { return (p_args ? p_args->a_argc : -1); }
    sArgList *arglist()   const { return (p_args); }
    void set_arglist(sArgList*a) { delete p_args; p_args = a; }
    
    bool collapsed()      const { return (p_collapsed); }
    void set_collapsed()        { p_collapsed = true; }

    bool readonly()       const { return (p_readonly); }
    void set_readonly()         { p_readonly = true; }

    void update(const sParam *p)
        {
            delete [] p_sub;
            p_sub = lstring::copy(p->p_sub);
            delete p_args;
            p_args = sArgList::copy(p->p_args);
            p_collapsed = p->p_collapsed;
            if (p->p_readonly)
                p_readonly = true;
        }

    inline void collapse(const sParamTab*);

private:
    char *p_name;       // token and flag
    char *p_sub;        // substitution text
    sArgList *p_args;   // function formal argument list
    bool p_collapsed;   // collapsed -> sub has been expanded
    bool p_readonly;    // value is immutable
};

struct sParamTab
{
    enum PTmode { PTparam, PTsubc, PTgeneral, PTsngl };
    // PTparam:     Strict handling of p=v constructs, fail on non p=v
    // PTsubc:      Handle p=v constructs only, setup for subckts
    // PTgeneral:   Handle p=v constructs only
    // PTsngl:      Handle p=v constructs plus isolated params and exprs

    struct PTctrl
    {
        // These flags are set transiently.  In the case of no_sqexp,
        // we want to set this without affecting const-ness, which is
        // why this flag is not in the main struct.  For collapse, the
        // subst function should become non-const when set, where by
        // const we assume no altering of the table entries.  This
        // flag could be in the main struct but is along for the ride
        // here.  These could be static, but that would be manifestly
        // thread-unsafe.

        PTctrl() : collapse(false), no_sqexp(false) { }

        bool collapse;       // Resolve and simplify definition chains.
        bool no_sqexp;       // Don't do single-quote expansion.
    };

    // Parameter names are case-insensitive!
    sParamTab()
        {
            pt_table = new sHtab(sHtab::get_ciflag(CSE_PARAM));
            pt_rctab = new sHtab(sHtab::get_ciflag(CSE_PARAM));
            pt_ctrl = new PTctrl;
            if (pt_set_predefs)
                (*pt_set_predefs)(this);
        }

    ~sParamTab();

    // tokenize str and update all tokens.
    //
    void param_subst_all(char **str) const
        {
#ifdef WRSPICE
            char *t = *str;
            // Treat 'set' and 'let' specially (for WRspice).
            if ((t[0] == 's' || t[0] == 'l') && t[1] == 'e' && t[2] == 't' &&
                    isspace(t[3]))
                defn_subst(this, str, PTgeneral, 1);
            else
#endif
            line_subst(str);
        }

    // Substitute and flatten internal references.
    //
    void param_subst_all_collapse(char **str)
        {
            pt_ctrl->collapse = true;
            line_subst(str);
            pt_ctrl->collapse = false;
        }

    // The str is expected to contain name = value definitions. 
    // Singleton tokens are either ignored or expanded according to
    // the second argument.
    //
    void param_subst_defn_list(char **str, bool do_singles) const
        {
            defn_subst(this, str, do_singles ? PTsngl : PTgeneral, 0);
        }

    // Special for .param lines.
    //
    void param_subst_param(char **str) const
        {
            defn_subst(this, str, PTparam, 1);
        }

    // Special for .subckt lines.
    //
    void param_subst_subckt(char **str) const
        {
            defn_subst(this, str, PTsubc, 2);
        }

    // Special for .model lines.
    //
    void param_subst_model(char **str) const
        {
            defn_subst(this, str, PTgeneral, 3);
        }

    // Special for .measure line.  For HSPICE compatibility, don't do
    // single-quote expansion.
    //
    void param_subst_measure(char **str) const
        {
            pt_ctrl->no_sqexp = true;
            line_subst(str);
            pt_ctrl->no_sqexp = false;
        }

    // Special for .options lines.
    //
    void param_subst_options(char **str)
        {
            pt_ctrl->collapse = true;
            defn_subst(this, str, PTgeneral, 1);
            pt_ctrl->collapse = false;
        }

    // Add a "pre-defined" read-only parameter.
    //
    void add_predef(const char *nm, const char *val)
        {
            if (nm) {
                sParam *prm = new sParam(lstring::copy(nm), lstring::copy(val));
                prm->set_readonly();
                pt_table->add(prm->name(), prm);
            }
        }

    static void register_set_predef_callback(void(*cb)(sParamTab*))
        { pt_set_predefs = cb; }

    static sParamTab *copy(const sParamTab*);
    static sParamTab *extract_params(sParamTab*, const char*);
    static sParamTab *update(sParamTab*, const sParamTab*);
    void update(const char*);
    double eval(const sParam*, bool*) const;
    void define_macros(bool = false);
    void undefine_macros();
    void dump() const;

    const sParam *get(const char *n) const
        { return ((const sParam*)sHtab::get(pt_table, n)); }

    unsigned int allocated()
        { return (pt_table ? pt_table->allocated() : 0); }

    static char *errString; // global error return

private:
    static void defn_subst(const sParamTab*, char**, PTmode, int);
    void line_subst(char**) const;
    void squote_subst(char**) const;
    bool subst(char**) const;
    void evaluate(char**) const;

    static bool tokenize(const char**, char**, char**, PTmode, const char** =0);

    sHtab *pt_table;        // Main table for elements.
    sHtab *pt_rctab;        // Used for recursion testing.
    PTctrl *pt_ctrl;        // Flags.

    static void(*pt_set_predefs)(sParamTab*);
};

// Mapping used when promoting macros.
//
struct sMacroMapTab : public sHtab
{
    sMacroMapTab() : sHtab(get_ciflag(CSE_UDF)) { }

    void add(const char *name, int argc, const char *newname)
        {
            sprintf(mmbuf, "%s:%d", name, argc);
            sHtab::add(mmbuf, (void*)newname);
        }

    const char *get(const char *name, int argc)
        {
            sprintf(mmbuf, "%s:%d", name, argc);
            return ((const char*)sHtab::get(this, mmbuf));
        }

private:
    char mmbuf[256];
};


inline void
sParam::collapse(const sParamTab *ptab)
{
    ptab->param_subst_all(&p_sub);
    p_collapsed = true;
}

#endif

