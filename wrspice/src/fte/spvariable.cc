
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "parser.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "graph.h"
#include "output.h"
#include "circuit.h"
#include "keywords.h"
#include "spnumber/spnumber.h"
#include <algorithm>


// The set command. Syntax is 
// set [opt ...] [opt = val ...]. Val may be a string, an int, a float, 
// or a list of the form (elt1 elt2 ...).
//
void
CommandTab::com_set(wordlist *wl)
{
    if (wl == 0) {
        Sp.VarPrint(0);
        return;
    }
    Sp.SetVar(wl);
}


// Unset command.  Syntax is
// unset word ...
//
void
CommandTab::com_unset(wordlist *wl)
{
    if (wl && lstring::eq(wl->wl_word, "*")) {
        Sp.ClearVariables();
        return;
    }
    while (wl != 0) {
        Sp.RemVar(wl->wl_word);
        wl = wl->wl_next;
    }
}
// Ena of CommandTab functions.


// Application code to set a boolean variable.
//
void
IFsimulator::SetVar(const char *varname)
{
    char *vname = lstring::copy(varname);
    CP.Unquote(vname);
    if (*vname == '&') {
        if (!vname[1])
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "attempt to set a variable named \"&\".  Maybe a backslash\n"
                "is needed to hide this character from the shell.\n");
        else
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "can't set a vector as a boolean.\n");
    }
    else {
        variable v;
        v.set_boolean(true);

        sKWent<userEnt> *entry =
            static_cast<sKWent<userEnt>*>(sHtab::get(ft_options, vname));
        if (entry) {
            entry->callback(true, &v);
            delete [] vname;
            return;
        }
        CP.RawVarSet(vname, true, &v);
    }
    delete [] vname;
}


// Application code to set an integer variable.
//
void
IFsimulator::SetVar(const char *varname, int value)
{
    char *vname = lstring::copy(varname);
    CP.Unquote(vname);
    if (*vname == '&') {
        if (!vname[1])
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "attempt to set a variable named \"&\".  Maybe a backslash\n"
                "is needed to hide this character from the shell.\n");
        else {
            char buf[64];
            sprintf(buf, "%d", value);
            OP.vecSet(vname+1, buf, false);
        }
    }
    else {
        variable v;
        v.set_integer(value);

        sKWent<userEnt> *entry =
            static_cast<sKWent<userEnt>*>(sHtab::get(ft_options, vname));
        if (entry) {
            entry->callback(true, &v);
            delete [] vname;
            return;
        }
        CP.RawVarSet(vname, true, &v);
    }
    delete [] vname;
}


// Application code to set a real variable.
//
void
IFsimulator::SetVar(const char *varname, double value)
{
    char *vname = lstring::copy(varname);
    CP.Unquote(vname);
    if (*vname == '&') {
        if (!vname[1])
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "attempt to set a variable named \"&\".  Maybe a backslash\n"
                "is needed to hide this character from the shell.\n");
        else {
            char buf[64];
            sprintf(buf, "%.16e", value);
            OP.vecSet(vname+1, buf, false);
        }
    }
    else {
        variable v;
        v.set_real(value);

        sKWent<userEnt> *entry =
            static_cast<sKWent<userEnt>*>(sHtab::get(ft_options, vname));
        if (entry) {
            entry->callback(true, &v);
            delete [] vname;
            return;
        }
        CP.RawVarSet(vname, true, &v);
    }
    delete [] vname;
}


// Application code to set a string variable.
//
void
IFsimulator::SetVar(const char *varname, const char *value)
{
    char *vname = lstring::copy(varname);
    CP.Unquote(vname);
    if (*vname == '&') {
        if (!vname[1])
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "attempt to set a variable named \"&\".  Maybe a backslash\n"
                "is needed to hide this character from the shell.\n");
        else
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "can't set a vector as a string.\n");
    }
    else {
        variable v;
        v.set_string(value);

        sKWent<userEnt> *entry =
            static_cast<sKWent<userEnt>*>(sHtab::get(ft_options, vname));
        if (entry) {
            entry->callback(true, &v);
            delete [] vname;
            return;
        }
        CP.RawVarSet(vname, true, &v);
    }
    delete [] vname;
}


// Application code to set a list variable.
//
void
IFsimulator::SetVar(const char *varname, variable *value)
{
    char *vname = lstring::copy(varname);
    CP.Unquote(vname);
    if (*vname == '&') {
        if (!vname[1])
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "attempt to set a variable named \"&\".  Maybe a backslash\n"
                "is needed to hide this character from the shell.\n");
        else
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "can't set a vector as a list.\n");
    }
    else {
        variable v;
        // The list is not copied, caller must not free!
        v.set_list(value);

        sKWent<userEnt> *entry =
            static_cast<sKWent<userEnt>*>(sHtab::get(ft_options, vname));
        if (entry) {
            entry->callback(true, &v);
            delete [] vname;
            return;
        }
        CP.RawVarSet(vname, true, &v);
    }
    delete [] vname;
}


// Parse the wordlist and set variables accordingly.
//
void
IFsimulator::SetVar(wordlist *wl)
{
    if (!wl)
        return;
    variable *vars = CP.ParseSet(wl);
    for (variable *v = vars; v; v = v->next()) {
        switch (v->type()) {
        case VTYP_BOOL:
            SetVar(v->name());
            break;
        case VTYP_NUM:
            SetVar(v->name(), v->integer());
            break;
        case VTYP_REAL:
            SetVar(v->name(), v->real());
            break;
        case VTYP_STRING:
            SetVar(v->name(), v->string());
            break;
        case VTYP_LIST:
            SetVar(v->name(), variable::copy(v->list()));
            break;
        default:
            break;
        }
    }
    variable::destroy(vars);
}


namespace {
    // Return the merging method for shell and circuit (set in
    // .options) variables.  This is a consequence of the values of
    // these variables:
    //    optmerge  ("global", "local", or "noshell")
    //    noshellopts (boolean, deprecated)
    // If noshellopts is given, optmerge is overridden, as if set to
    // "noshell".
    //
    // The default is OMRG_GLOBAL, which causes checking of the shell
    // variable first, then circuit variables if any.  This is the
    // return if there is no circ.
    // 
    // Otherwise, we look at the options structs for the variable
    // sets.  If either is OMRG_NOSHELL, that value is returned. 
    // Then, if either is OMRG_LOCAL, that value is returned. 
    // Otherwise, return the default OMRG_GLOBAL.
    //
    OMRG_TYPE merge_type(sFtCirc *circ)
    {
        if (circ && circ->defOpt()) {
            sOPTIONS *shell_opts = sOPTIONS::shellOpts();
            OMRG_TYPE sh_mt = OMRG_GLOBAL;
            if (shell_opts->OPTnoshellopts_given)
                sh_mt = OMRG_NOSHELL;
            else if (shell_opts->OPToptmerge_given)
                sh_mt = (OMRG_TYPE)shell_opts->OPToptmerge;
            if (sh_mt == OMRG_NOSHELL)
                return (OMRG_NOSHELL);

            sOPTIONS *ckt_opts = circ->defOpt();
            OMRG_TYPE ct_mt = OMRG_GLOBAL;
            if (ckt_opts->OPTnoshellopts_given)
                ct_mt = OMRG_NOSHELL;
            else if (ckt_opts->OPToptmerge_given)
                ct_mt = (OMRG_TYPE)ckt_opts->OPToptmerge;
            if (ct_mt == OMRG_NOSHELL)
                return (OMRG_NOSHELL);
            if (sh_mt == OMRG_LOCAL || ct_mt == OMRG_LOCAL)
                return (OMRG_LOCAL);
        }
        return (OMRG_GLOBAL);
    }
}


// Return the variable struct for the variable name given.  If circ is
// passed, the circuit variables will be tested as well, according to
// the current merging rule.  If circ is nil and the variable
// kw_cktvars is set in the shell, circ is effectively set to the
// current circuit.
//
variable *
IFsimulator::GetRawVar(const char *name, sFtCirc *circ)
{
    if (!circ && CP.RawVarGet(kw_cktvars))
        circ = ft_curckt;

    // Merging: Applies when circ != null
    // OMRG_GLOBAL:  check shell, then circuit if any (default)
    // OMRG_LOCAL:   check circuit, then shell
    // OMRG_NOSHELL: check the circuit only

    OMRG_TYPE mt = merge_type(circ);
    if (mt != OMRG_GLOBAL) {
        if (sHtab::get(ft_options, name)) {
            for (variable *v = circ->vars(); v; v = v->next()) {
                if (lstring::cieq(v->name(), name))
                    return (v);
            }
        }
        else {
            for (variable *v = circ->vars(); v; v = v->next()) {
                if (lstring::eq(v->name(), name))
                    return (v);
            }
        }
    }
    if (mt != OMRG_NOSHELL) {
        variable *v = CP.RawVarGet(name);
        if (v)
            return (v);
    }
    if (circ && mt == OMRG_GLOBAL) {
        if (sHtab::get(ft_options, name)) {
            for (variable *v = circ->vars(); v; v = v->next()) {
                if (lstring::cieq(v->name(), name))
                    return (v);
            }
        }
        else {
            for (variable *v = circ->vars(); v; v = v->next()) {
                if (lstring::eq(v->name(), name))
                    return (v);
            }
        }
    }
    return (0);
}


// Determine the value of a variable.  Fail if the variable is unset,
// and if the type doesn't match, try and make it work.  If circ is
// passed, consider the circuit variables using the current merging. 
// If circ is nil and the variable kw_cktvars is set in the shell,
// circ is effectively set to the current circuit.
//
bool
IFsimulator::GetVar(const char *name, VTYPenum type, VTvalue *retval,
    sFtCirc *circ)
{
    variable *v = GetRawVar(name, circ);
    if (retval == 0) {
        if (v && (type == v->type() || type == VTYP_BOOL))
            return (true);
        return (false);
    }
    retval->reset();
    if (v == 0)
        return (false);

    if (v->type() == type) {
        switch (type) {
        case VTYP_BOOL:
            retval->set_bool(true);
            break;
        case VTYP_NUM:
            retval->set_int(v->integer());
            break;
        case VTYP_REAL:
            retval->set_real(v->real());
            break;
        case VTYP_STRING:
            {
                char *s = lstring::copy(v->string());
                CP.Unquote(s);
                CP.Strip(s);
                if (!s)
                    s = lstring::copy("");
                retval->set_string(s);
            }
            break;
        case VTYP_LIST: // Funny case...
            retval->set_list(v->list());
            break;
        default:
            GRpkgIf()->ErrPrintf(ET_INTERR, "getvar: bad var type %d.\n",
                type);
            break;
        }
        return (true);
    }
    else {
        // Try to coerce it..
        char buf[BSIZE_SP];
        if (type == VTYP_NUM && v->type() == VTYP_REAL) {
            retval->set_int((int)v->real());
            return (true);
        }
        if (type == VTYP_REAL && v->type() == VTYP_NUM) {
            retval->set_real((double)v->integer());
            return (true);
        }
        if (type == VTYP_STRING && v->type() == VTYP_NUM) {
            sprintf(buf, "%d", v->integer());
            retval->set_string(lstring::copy(buf));
            return (true);
        }
        if (type == VTYP_STRING && v->type() == VTYP_REAL) {
            sprintf(buf, "%g", v->real());
            retval->set_string(lstring::copy(buf));
            return (true);
        }
        if (type == VTYP_STRING && v->type() == VTYP_LIST) {
            char *s = lstring::copy("(");
            for (variable *vv = v->list(); vv; vv = vv->next()) {
                switch (vv->type()) {
                case VTYP_NUM:
                    sprintf(buf, " %d", vv->integer());
                    s = lstring::build_str(s, buf);
                    break;
                case VTYP_REAL:
                    sprintf(buf, " %g", vv->real());
                    s = lstring::build_str(s, buf);
                    break;
                case VTYP_STRING:
                    sprintf(buf, " %s", vv->string());
                    s = lstring::build_str(s, buf);
                    break;
                default:
                    break;
                }
            }
            s = lstring::build_str(s, " )");
            retval->set_string(s);
            return (true);
        }
        if ((type == VTYP_REAL || type == VTYP_NUM) &&
                v->type() == VTYP_STRING) {
            const char *s = v->string();
            double *d = SPnum.parse(&s, false);
            if (d && *s == '\0') {
                if (type == VTYP_REAL)
                    retval->set_real(*d);
                else
                    retval->set_int((int)*d);
            }
            return (true);
        }
        return (false);
    }
}


namespace {
    struct sort_elt
    {
        variable *x_v;
        char x_char;
        unsigned char x_sval;
    };

    bool vcmp(const sort_elt &v1, const sort_elt &v2)
    {
        int i = strcmp(v1.x_v->name(), v2.x_v->name());
        if (i)
            return (i < 0);
        return (v1.x_sval < v2.x_sval);
    }
}


// Print the values of currently defined variables.
//
void
IFsimulator::VarPrint(char **retstr)
{
    // Copy the list of current circuit variables.
    variable *cktvars = 0;
    if (CurCircuit())
        cktvars = variable::copy(CurCircuit()->vars());
    if (CurAnalysis()) {
        variable *vv = new variable(kw_curanalysis);
        vv->set_string(CurAnalysis()->name);
        vv->set_next(cktvars);
        cktvars = vv;
    }

    // List the plot variables.
    variable *plvars = 0;
    if (OP.curPlot())
        plvars = variable::copy(OP.curPlot()->environment());
    {
        variable *tv;
        if ((tv = EnqPlotVar(kw_plots)) != 0) {
            tv->set_next(plvars);
            plvars = tv;
        }
        if ((tv = EnqPlotVar(kw_curplot)) != 0) {
            tv->set_next(plvars);
            plvars = tv;
        }
        if ((tv = EnqPlotVar(kw_curplottitle)) != 0) {
            tv->set_next(plvars);
            plvars = tv;
        }
        if ((tv = EnqPlotVar(kw_curplotname)) != 0) {
            tv->set_next(plvars);
            plvars = tv;
        }
        if ((tv = EnqPlotVar(kw_curplotdate)) != 0) {
            tv->set_next(plvars);
            plvars = tv;
        }
        if ((tv = EnqPlotVar(kw_display)) != 0) {
            tv->set_next(plvars);
            plvars = tv;
        }
    }

    OMRG_TYPE mt = merge_type(CurCircuit());
    // The in-scope value is listed first in the sort order when there
    // are duplicates.  The OMRG_NOSHELL case is treated as OMRG_LOCAL
    // for the listing.

    variable *v;
    int i = CP.VarDb()->allocated();
    for (v = plvars; v; v = v->next())
        i++;
    for (v = cktvars; v; v = v->next())
        i++;

    sort_elt *vars = new sort_elt[i];

    i = 0;
    sHgen gen(CP.VarDb()->table());
    sHent *h;
    while ((h = gen.next()) != 0) {
        vars[i].x_v = (variable*)h->data();
        vars[i].x_char = ' ';
        vars[i].x_sval = (mt == OMRG_GLOBAL ? 0 : 1);
        i++;
    }
    for (v = plvars; v; v = v->next(), i++) {
        vars[i].x_v = v;
        vars[i].x_char = '*';
        vars[i].x_sval = (mt == OMRG_GLOBAL ? 1 : 2);
    }
    for (v = cktvars; v; v = v->next(), i++) {
        vars[i].x_v = v;
        vars[i].x_char = '+';
        vars[i].x_sval = (mt == OMRG_GLOBAL ? 2 : 0);
    }
    std::sort(vars, vars + i, vcmp);

    if (!retstr)
        TTY.send("\n");
    for (int j = 0; j < i; j++) {
        char buf[1024];
        if (j && lstring::eq(vars[j].x_v->name(), vars[j - 1].x_v->name()))
            continue;
        v = vars[j].x_v;
        if (v->type() == VTYP_BOOL) {
            const char *fmt = "%c %-18s\n";
            if (!retstr)
                TTY.printf(fmt, vars[j].x_char, v->name());
            else {
                sprintf(buf, fmt, vars[j].x_char, v->name());
                *retstr = lstring::build_str(*retstr, buf);
            }
        }
        else {
            const char *fmt = "%c %-18s";
            if (!retstr) 
                TTY.printf("%c %-18s", vars[j].x_char, v->name());
            else
                sprintf(buf, "%c %-18s", vars[j].x_char, v->name());

            wordlist *wl = v->varwl();
            char *s = wordlist::flatten(wl);
            wordlist::destroy(wl);
            if (v->type() == VTYP_LIST) {
                fmt = "( %s )\n";
                if (!retstr)
                    TTY.printf(fmt, s);
                else {
                    sprintf(buf + strlen(buf), fmt, s);
                    *retstr = lstring::build_str(*retstr, buf);
                }
            }
            else {
                if (!retstr)
                    TTY.printf("%s\n", s);
                else {
                    sprintf(buf + strlen(buf), "%s\n", s);
                    *retstr = lstring::build_str(*retstr, buf);
                }
            }
            delete [] s;
        }
    }
    if (!retstr)
        TTY.send("\n");
    variable::destroy(plvars);
    variable::destroy(cktvars);
    delete [] vars;
}


namespace {
    variable *getlist(sDataVec *d, int dim, void **x)
    {
        variable *vv = 0, *tv = 0;
        for (int i = d->dims(dim) - 1; i >= 0; i--) {
            if (!tv)
                tv = vv = new variable;
            else {
                vv->set_next(new variable);
                vv = vv->next();
            }
            if (dim == d->numdims()-1) {
                if (d->isreal()) {
                    double *dd = *(double**)x;
                    vv->set_real(*dd);
                    dd++;
                    *x = (void*)dd;
                }
                else {
                    complex *cc = *(complex**)x;
                    vv->set_real(cc->real);
                    cc++;
                    *x = (void*)cc;
                }
            }
            else
                vv->set_list(getlist(d, dim+1, x));
        }
        return (tv);
    }


    variable *vec2var(sDataVec *d, int lo, int hi)
    {
        variable *vv = 0, *tv = 0;
        int sn = ((hi < lo) ? -1 : 1);
        int sz = d->length()/d->dims(0);
        for (int i = lo; ((sn == 1) ? (i <= hi) : (i >= hi)); i += sn) {
            if (i >= d->dims(0))
                continue;
            if (!tv)
                tv = vv = new variable;
            else {
                vv->set_next(new variable);
                vv = vv->next();
            }
            if (d->numdims() == 1)
                vv->set_real(d->realval(i));
            else {
                void *x;
                if (d->isreal())
                    x = static_cast<void*>(&d->realvec()[i*sz]);
                else
                    x = static_cast<void*>(&d->compvec()[i*sz]);
                vv->set_list(getlist(d, 1, &x));
            }
        }
        if (tv) {
            if (lo == hi)
                return (tv);
            vv = new variable;
            vv->set_list(tv);
            return (vv);
        }
        return (0);
    }
}


// If name matches a plot variable, return an allocated variable
// struct for the variable, caller should free.
//
variable *
IFsimulator::EnqPlotVar(const char *name)
{
    if (!name)
        return (0);
    if (*name == '$')
        name++;
    variable *vv;
    if (lstring::eq(name, kw_display)) {
        vv = new variable(name);
        vv->set_string(CP.Display());
        return (vv);
    }
    if (CurAnalysis()) {
        if (lstring::eq(name, kw_curanalysis)) {
            vv = new variable(name);
            vv->set_string(CurAnalysis()->name);
            return (vv);
        }
    }
    if (OP.curPlot()) {
        for (vv = OP.curPlot()->environment(); vv; vv = vv->next())
            if (lstring::eq(vv->name(), name))
                break;
        if (vv)
            vv =  variable::copy(vv);
        else if (lstring::eq(name, kw_curplotname)) {
            vv = new variable(name);
            vv->set_string(OP.curPlot()->name());
        }
        else if (lstring::eq(name, kw_curplottitle)) {
            vv = new variable(name);
            vv->set_string(OP.curPlot()->title());
        }
        else if (lstring::eq(name, kw_curplotdate)) {
            vv = new variable(name);
            vv->set_string(OP.curPlot()->date());
        }
        else if (lstring::eq(name, kw_curplot)) {
            vv = new variable(name);
            vv->set_string(OP.curPlot()->type_name());
        }
        else if (lstring::eq(name, kw_plots)) {
            vv = new variable(name);
            for (sPlot *pl = OP.plotList(); pl; pl = pl->next_plot()) {
                variable *tv = new variable;
                tv->set_string(pl->type_name());
                tv->set_next(vv->list());

                // Careful! set_list frees existing list.
                vv->zero_list();
                vv->set_list(tv);
            }
        }
        if (vv)
            return (vv);
    }
    return (0);
}


// Return a copy of the internal variable struct if name matches a
// circuit valiable.
//
variable *
IFsimulator::EnqCircuitVar(const char *name)
{
    if (name && CurCircuit()) {
        if (*name == '$')
            name++;
        if (sHtab::get(ft_options, name)) {
            // The name matches one of the built-in option names,
            // which are case-insensitive.

            for (variable *v = CurCircuit()->vars(); v; v = v->next()) {
                if (lstring::cieq(v->name(), name))
                    return (variable::copy(v));
            }
        }
        else {
            for (variable *v = CurCircuit()->vars(); v; v = v->next()) {
                if (lstring::eq(v->name(), name))
                    return (variable::copy(v));
            }
        }
    }
    return (0);
}


// Function to query the values of variables that use the $&varname
// notation.  This function accepts a range extension for $&
// variables.  It will also evaluate vector expressions encased in (
// ).
//
// The varcheck shuts up "not found" messages, used when checking for
// existence only.
//
// The return is an allocated variable, caller should free when done.
//
variable *
IFsimulator::EnqVectorVar(const char *word, bool varcheck)
{
    if (!word)
        return (0);
    if (*word == '$')
        word++;
    if (*word == '&') {
        word++;
        int low = 0;
        int up = -1;
        const char *range, *r;
        const char *tt = 0;
        if (*word == '(') {
            // an expression
            tt = strrchr(word,')');
            if (tt == 0) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "closing ')' not found.\n");
                return (0);
            }
            r = range = strrchr(tt, '[');
        }
        else
            r = range = strrchr(word, '[');

        if (r) {
            // Parse the range only if not a 'special' vector, or if it
            // is, then the range must come after the [param] field.
            // see note in VecGet().
            //
            if (*word != SpecCatchar() || r != strchr(word, '[')) {
                r++;
                if (!isdigit(*r) && *r != '-')
                    GRpkgIf()->ErrPrintf(ET_WARN,
                        "nonparseable range specified, %s[%s.\n", word, r);
                for (low = 0; isdigit(*r); r++)
                    low = low * 10 + *r - '0';
                if ((*r == '-') && isdigit(r[1]))
                    for (up = 0, r++; isdigit(*r); r++)
                        up = up * 10 + *r - '0';
                else if (*r != '-')
                    up = low;
            }
        }

        sDataVec *d = 0;
        unsigned int length = 0;
        unsigned int exist = 0;
        char *word_strp = lstring::copy(word);
        if (range && (*word != SpecCatchar()))
            word_strp[range - word] = 0;
        if (tt) {
            // evaluate the expression
            const char *stmp = word_strp;
            pnode *pn = GetPnode(&stmp, true);
            if (pn == 0) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "parse failed.\n");
                return (0);
            }
            d = Evaluate(pn);
            delete pn;
        }
        else {
            sCKT *ckt = ft_curckt ? ft_curckt->runckt() : 0;
            if (*word_strp == '?') {
                exist = 1;
                if (OP.isVec(word_strp+1, ckt))
                    exist++;
            }
            else if (*word_strp == '#') {
                d = OP.vecGet(word_strp+1, ckt, true);
                if (d) {
                    length = d->length();
                    d = 0;
                }
            }
            else {
                d = OP.vecGet(word_strp, ckt, varcheck);
            }
        }
        delete [] word_strp;

        variable *vv = 0;
        if (d) {
            if (d->link()) {
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "only one vector may be accessed with the $& notation.\n");
                d = d->link()->dl_dvec;
            }
            if (d->flags() & VF_STRING) {
                // The parameter has string type, is is passed in the
                // defcolor field.

                vv = new variable(word);
                vv->set_string(d->defcolor());
                return (vv);
            }
            if (d->numdims() <= 1) {
                d->set_numdims(1);
                d->set_dims(0, d->length());
            }
            if (up == -1)
                up = d->dims(0) - 1;
            vv = vec2var(d, low, up);
        }
        else if (exist) {
            vv = new variable;
            vv->set_boolean(exist == 2);
        }
        else if (length) {
            vv = new variable;
            vv->set_integer(length);
        }
        return (vv);
    } 
    return (0);
}


// Application code to unset (and remove from list) a variable
//
void
IFsimulator::RemVar(const char *varname)
{
    char vname[BSIZE_SP];
    strcpy(vname, varname);
    CP.Unquote(vname);
    sKWent<userEnt> *entry =
        static_cast<sKWent<userEnt>*>(sHtab::get(ft_options, vname));
    if (entry) {
        entry->callback(false, 0);
        return;
    }
    CP.RawVarSet(vname, false, 0);
}


// Clear the variables database
//
void
IFsimulator::ClearVariables()
{
    wordlist *wl0 = CP.VarList();
    for (wordlist *tl = wl0; tl; tl = tl->wl_next)
        RemVar(tl->wl_word);
    wordlist::destroy(wl0);
}

