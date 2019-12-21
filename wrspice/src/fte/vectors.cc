
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
#include "rawfile.h"
#include "csdffile.h"
#include "prntfile.h"
#include "parser.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "commands.h"
#include "toolbar.h"
#include "circuit.h"
#include "optdefs.h"
#include "statdefs.h"
#include "errors.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"


//
// Functions for dealing with the vector database.
//

namespace {
    // If vname is in the form plot<SEPARATOR>vector, strip off the
    // plot, if it is found, and return the sPlot*.  If the plot is
    // "all", set the flag.
    //
    sPlot *stripplot(const char **vname, bool *allflag)
    {
        *allflag = false;
        const char *s = strchr(*vname, Sp.PlotCatchar());
        if (s && s != *vname) {
            int ix = s - *vname;
            char *bf = new char[ix + 1];
            strncpy(bf, *vname, ix);
            bf[ix] = 0;
            if (lstring::cieq(bf, "all")) {
                *allflag = true;
                *vname = ++s;
                delete [] bf;
                return (0);
            }
            sPlot *p = OP.findPlot(bf);
            delete [] bf;
            if (p) {
                *vname = ++s;
                return (p);
            }
        }
        return (0);
    }
}


// This can now handle multiple assignments on one line:
// let a = b c = d ...
//
void
CommandTab::com_let(wordlist *wl)
{
    if (!wl) {
        OP.vecPrintList(0, 0);
        return;
    }
    char *p = wordlist::flatten(wl);

    char *rhs = strchr(p, '=');
    if (!rhs) {
        OP.vecPrintList(wl, 0);
        delete [] p;
        return;
    }
    char *lhs = p;
    for (;;) {
        const char *badchrs = "|&><\"\'`$+-*/^%,!~\\";

        *rhs++ = 0;
        char *q = rhs-2;
        while (*q <= ' ' && q >= lhs)
           *q-- = '\0';
        if (lhs > q) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "no vector to assign.\n");
            delete [] p;
            return;
        }
        while (isspace(*rhs))
            rhs++;
        if (!*rhs) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "no right hand side in assignment to %s.\n", lhs);
            delete [] p;
            return;
        }

        // Apply some reasonable limits to the characters that can be
        // included in vector names.

        for (char *t = lhs; *t; t++) {
            if (*t <= ' ' || strchr(badchrs, *t)) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "the name %s contains unaccepted character \'%c\'.\n",
                    lhs, *t);
                delete [] p;
                return;
            }
        }

        const char *rhs_residue;
        OP.vecSet(lhs, rhs, false, &rhs_residue);
        if (rhs_residue == rhs)
            break;
        while (isspace(*rhs_residue))
            rhs_residue++;
        if (!*rhs_residue)
            break;
        lhs = p + (rhs_residue - p);
        rhs = strchr(lhs, '=');
        if (!rhs)
            break;
    }
    delete [] p;
}


// Command to remove vectors from plots.  This supports the forms
// vector, plot.vector, all.vector, all, plot.all, all.all
//
void
CommandTab::com_unlet(wordlist *wl)
{
    if (!wl)
        return;
    ToolBar()->UpdateVectors(1);
    for ( ; wl; wl = wl->wl_next) {
        char buf[BSIZE_SP];
        strcpy(buf, wl->wl_word);
        const char *s = buf;
        bool allflag = false;
        sPlot *pl = stripplot(&s, &allflag);
        if (pl)
            pl->remove_vec(s);
        else if (allflag) {
            for (pl = OP.plotList(); pl; pl = pl->next_plot())
                pl->remove_vec(s);
        }
        else
            OP.curPlot()->remove_vec(s);
    }
    ToolBar()->UpdateVectors(1);
}


// Display vector status, etc.  Note that this only displays stuff from the
// current plot, and you must do a setplot to see the rest of it.
//
void
CommandTab::com_display(wordlist *wl)
{
    OP.vecPrintList(wl, 0);
}


// Take a set of vectors and form a new vector of the nth elements of each.
//
void
CommandTab::com_cross(wordlist *wl)
{
    char *newvec = wl->wl_word;
    wl = wl->wl_next;
    const char *s = wl->wl_word;
    double *d;
    if (!(d = SPnum.parse(&s, false))) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad number %s.\n", wl->wl_word);
        return;
    }
    int ind;
    if ((ind = (int)*d) < 0) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad index %d.\n", ind);
        return;
    }
    wl = wl->wl_next;
    pnlist *pl0 = Sp.GetPtree(wl, true);
    if (pl0 == 0)
        return;

    sDvList *dl0 = 0, *dl = 0;
    for (pnlist *pl = pl0; pl; pl = pl->next()) {
        sDataVec *n = Sp.Evaluate(pl->node());
        if (n == 0) {
            pnlist::destroy(pl0);
            sDvList::destroy(dl0);
            return;
        }
        if (!dl0)
            dl0 = dl = new sDvList;
        else {
            dl->dl_next = new sDvList;
            dl = dl->dl_next;
        }
        if (n->link()) {
            for (sDvList *tl = n->link(); tl; tl = tl->dl_next) {
                dl->dl_dvec = tl->dl_dvec;
                if (tl->dl_next) {
                    dl->dl_next = new sDvList;
                    dl = dl->dl_next;
                }
            }
        }
        else
            dl->dl_dvec = n;
    }
    pnlist::destroy(pl0);

    int i = 0;
    bool comp = false;
    for (dl = dl0; dl; dl = dl->dl_next) {
        if (dl->dl_dvec->iscomplex())
            comp = true;
        i++;
    }

    newvec = lstring::copy(newvec);
    CP.Unquote(newvec);
    sDataVec *v = new sDataVec(newvec, comp ? VF_COMPLEX : 0, i,
        dl0->dl_dvec ? dl0->dl_dvec->units() : 0);
    v->newperm();
    
    // Now copy the ind'ths elements into this one
    for (dl = dl0, i = 0; dl; dl = dl->dl_next, i++) {
        sDataVec *n = dl->dl_dvec;
        if (n->length() > ind) {
            if (comp)
                v->set_compval(i, n->compval(ind));
            else
                v->set_realval(i, n->realval(ind));
        }
        else {
            if (comp) {
                v->set_realval(i, 0.0);
                v->set_imagval(i, 0.0);
            }
            else
                v->set_realval(i, 0.0);
        }
    }
    CP.AddKeyword(CT_VECTOR, v->name());
    sDvList::destroy(dl0);
    ToolBar()->UpdateVectors(0);
}


// pick vecname offset period vector vector...
// Create vecname consisting of the period'th values of the vectors starting
// at offset
void
CommandTab::com_pick(wordlist *wl)
{
    char *newvec = wl->wl_word;
    wl = wl->wl_next;
    const char *s = wl->wl_word;
    double *d;
    if (!(d = SPnum.parse(&s, false))) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad offset %s.\n", wl->wl_word);
        return;
    }
    int offset;
    if ((offset = (int)*d) < 0) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad offset %d.\n", offset);
        return;
    }
    wl = wl->wl_next;

    s = wl->wl_word;
    if (!(d = SPnum.parse(&s, false))) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad period %s.\n", wl->wl_word);
        return;
    }
    int period;
    if ((period = (int)*d) < 1) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad period %d.\n", period);
        return;
    }
    wl = wl->wl_next;

    pnlist *pl0 = Sp.GetPtree(wl, true);
    if (pl0 == 0)
        return;

    sDvList *dl0 = 0, *dl = 0;
    for (pnlist *pl = pl0; pl; pl = pl->next()) {
        sDataVec *n = Sp.Evaluate(pl->node());
        if (n == 0) {
            pnlist::destroy(pl0);
            sDvList::destroy(dl0);
            return;
        }
        if (!dl0)
            dl0 = dl = new sDvList;
        else {
            dl->dl_next = new sDvList;
            dl = dl->dl_next;
        }
        if (n->link()) {
            for (sDvList *tl = n->link(); tl; tl = tl->dl_next) {
                dl->dl_dvec = tl->dl_dvec;
                if (tl->dl_next) {
                    dl->dl_next = new sDvList;
                    dl = dl->dl_next;
                }
            }
        }
        else
            dl->dl_dvec = n;
    }
    pnlist::destroy(pl0);

    bool comp = false;
    int len = 0;
    int num = 0;
    for (dl = dl0; dl; dl = dl->dl_next) {
        if (dl->dl_dvec->length() > len)
          len = dl->dl_dvec->length();
        if (dl->dl_dvec->iscomplex())
            comp = true;
        num++;
    }
    int nlen = len;
    nlen /= period;

    newvec = lstring::copy(newvec);
    CP.Unquote(newvec);
    sDataVec *v = new sDataVec(newvec, comp ? VF_COMPLEX : 0,
        num*nlen, dl0->dl_dvec ? dl0->dl_dvec->units() : 0);
    v->newperm();
    
    int j = 0;
    for (int i = offset; i < len; i += period) {
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *n = dl->dl_dvec;
            if (i < n->length()) {
                if (comp)
                    v->set_compval(j, n->compval(i));
                else
                    v->set_realval(j, n->realval(i));
            }
            else {
                if (comp) {
                    v->set_realval(j, 0.0);
                    v->set_imagval(j, 0.0);
                }
                else
                    v->set_realval(j, 0.0);
            }
            j++;
        }
    }

    CP.AddKeyword(CT_VECTOR, v->name());
    sDvList::destroy(dl0);
    ToolBar()->UpdateVectors(0);
}


// Command to assign default scale to plots or vectors
//   setscale vector      : makes vector the scale for the current plot
//   setscale plot vector : makes vector the scale for plot
//   setscale vec1 vec2 ... vector : makes vector the scale for vec1...
//
void
CommandTab::com_setscale(wordlist *wl)
{
    if (!wl)
        return;
    const char *errm = "no such vector %s.\n";
    if (!wl->wl_next) {
        // set current plot scale
        sDataVec *v = OP.curPlot()->find_vec(wl->wl_word);
        if (v) {
            OP.curPlot()->set_scale(v);
            ToolBar()->UpdateVectors(0);
            return;
        }
        GRpkgIf()->ErrPrintf(ET_ERROR, errm, wl->wl_word);
        return;
    }

    // If the first word matches a plot, set the plot's scale
    wordlist *sl = wl->wl_next;
    while (sl->wl_next)
        sl = sl->wl_next;
    if (wl->wl_next == sl) {
        sPlot *pl = OP.findPlot(wl->wl_word);
        if (pl) {
            sDataVec *v = pl->find_vec(sl->wl_word);
            if (v) {
                pl->set_scale(v);
                if (pl == OP.curPlot())
                    ToolBar()->UpdateVectors(0);
                return;
            }
            GRpkgIf()->ErrPrintf(ET_ERROR, errm, sl->wl_word);
            return;
        }
    }
    sPlot *pl = OP.curPlot();
    sDataVec *sc = 0;
    if (!lstring::cieq(sl->wl_word, "none") &&
            !lstring::cieq(sl->wl_word, "default")) {
        sc = pl->find_vec(sl->wl_word);
        if (!sc) {
            GRpkgIf()->ErrPrintf(ET_ERROR, errm, sl->wl_word);
            return;
        }
        if (sc == pl->scale())
            sc = 0;
    }
    for ( ; wl != sl; wl = wl->wl_next) {
        sDataVec *v = pl->find_vec(wl->wl_word);
        if (!v) {
            GRpkgIf()->ErrPrintf(ET_WARN, errm, wl->wl_word);
            continue;
        }
        v->set_scale(sc);
    }
    if (pl == OP.curPlot())
        ToolBar()->UpdateVectors(0);
}
// End of CommandTab functions.


// This is the main lookup function for names. The possible types of names are:
//  name           An ordinary vector.
//  plot.name      A vector from a particular plot.
//  @device[parm]  A device parameter.
//  @model[parm]   A model parameter.
//  @param         A circuit parameter.
// For the @ cases, we construct a dvec with length 1 to hold the value.
// In the other two cases, either the plot or the name can be "all", a
// wildcard.
// The vector name may have embedded dots -- if the first component is a plot
// name, it is considered the plot, otherwise the current plot is used.
//
// The ckt argument is needed to resolve the '@'  "special" cases, and
// "temper".
//
sDataVec *
IFoutput::vecGet(const char *word, const sCKT *ckt, bool silent)
{
    if (!word || !*word) {
        if (!silent) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "vecGet, null or empty vector name encountered.\n");
        }
        return (0);
    }
    sPlot *pl = curPlot();
    sDataVec *d = pl->find_vec(word);
    if (d)
        return (d);

    bool allflag;
    const char *tw = word;
    pl = stripplot(&word, &allflag);
    if (pl) {
        d = pl->find_vec(word);
        if (d)
            return (d);
        word = tw;
        pl = curPlot();
    }
    else if (allflag) {
        // plot is wildcard
        sDvList *dl0 = 0, *dl = 0;
        for (pl = plotList(); pl; pl = pl->next_plot()) {
            if (pl == o_constants)
                continue;
            d = pl->find_vec(word);
            if (d) {
                if (dl0 == 0)
                    dl0 = dl = new sDvList;
                else {
                    dl->dl_next = new sDvList;
                    dl = dl->dl_next;
                }
                if (d->link()) {
                    for (sDvList *dll = d->link(); dll; dll = dll->dl_next) {
                        dl->dl_dvec = dll->dl_dvec;
                        if (dll->dl_next) {
                            dl->dl_next = new sDvList;
                            dl = dl->dl_next;
                        }
                    }
                }
                else
                    dl->dl_dvec = d;
            }
        }
        if (!dl0) {
            if (!silent)
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "plot wildcard (name %s) matches nothing.\n", word);
            return (0);
        }
        d = new sDataVec(lstring::copy("list"), 0, 0);
        d->set_link(dl0);
        d->newtemp();
        d->sort();
        return (d);
    }
    else
        pl = curPlot();

    if (cxPlot() && cxPlot() != pl && cxPlot() != o_constants)
        d = cxPlot()->find_vec(word);
    if (!d)
        d = o_constants->find_vec(word);
    if (d)
        return (d);

    char *name = lstring::copy(word);
    char *param;
    for (param = name; *param && (*param != '['); param++) ;
    if (*param) {
        *param++ = '\0';
        char *s;
        for (s = param; *s && *s != ']'; s++) ;
        *s = '\0';
    }
    else
        param = 0;
    IFdata data;
    if (*word == Sp.SpecCatchar()) {
        // This is a special quantity...
        int err = 1;
        if (param && *param) {
            if (ckt) {
                err = ckt->getParam(name+1, param, &data);
                if (err)
                    err = ckt->getAnalParam(name+1, param, &data);
            }
        }
        else {
            if (ckt) {
                err = OPTinfo.getOpt(ckt, 0, name+1, &data);
                if (err)
                    err = ckt->CKTbackPtr->dotparam(name+1, &data);
                if (err) {
                    err = STATinfo.getOpt(ckt, 0, name+1, &data);
                }
            }
            if (err) {
                if (lstring::eq(name+1, kw_elapsed)) {
                    double d1, d2, d3;
                    ResPrint::get_elapsed(&d1, &d2, &d3);
                    data.type = IF_REAL;
                    data.v.rValue = d1;
                    err = 0;
                }
                else if (lstring::eq(name+1, kw_space)) {
                    unsigned int d1, d2, d3;
                    ResPrint::get_space(&d1, &d2, &d3);
                    data.type = IF_INTEGER;
                    data.v.iValue = d1;
                    err = 0;
                }
            }
        }
        if (err) {
            if (!silent)
                GRpkgIf()->ErrPrintf(ET_ERROR, "could not evaluate %s.\n",
                    word);
            delete [] name;
            return (0);
        }
    }
    else {
        bool ok = (ckt && ckt->CKTbackPtr->getVerilog(name, param, &data));
        // If ok is true, name was resolved in Verilog block.

        if (!ok) {
            // Look for some global names.  The only one supported
            // so far is "temper" which returns the current
            // temperature in Celsius, same as HSPICE.

            if (lstring::eq(word, "temper")) {
                data.type = IF_REAL;
                VTvalue vv;
                if (ckt && ckt->CKTcurTask)
                    data.v.rValue = ckt->CKTcurTask->TSKtemp - wrsCONSTCtoK;
                else if (Sp.GetVar(spkw_temp, VTYP_REAL, &vv))
                    data.v.rValue = vv.get_real();
                else
                    data.v.rValue = wrsREFTEMP - wrsCONSTCtoK;
                ok = true;
            }
        }
        if (!ok) {
            // Name can't be resolved.
            delete [] name;
            return (0);
        }
    }

    delete [] name;
    d = new sDataVec(data.toUU());
    d->set_name(word);
    d->newtemp(pl);
    int dtype = data.type & IF_VARTYPES;
    dtype &= ~IF_VECTOR;
    if (dtype == IF_FLAG) {
        if (data.type & IF_VECTOR) {
            d->set_length(data.v.v.numValue);
            d->set_realvec(new double[d->length()]);
            for (int j = 0; j < d->length(); j++)
                d->set_realval(j, (double)data.v.v.vec.iVec[j]);
        }
        else {
            d->set_realvec(new double[1]);
            d->set_length(1);
            d->set_realval(0, (double)data.v.iValue);
        }
    }
    else if (dtype == IF_INTEGER) {
        if (data.type & IF_VECTOR) {
            d->set_length(data.v.v.numValue);
            d->set_realvec(new double[d->length()]);
            for (int j = 0; j < d->length(); j++)
                d->set_realval(j, (double)data.v.v.vec.iVec[j]);
        }
        else {
            d->set_realvec(new double[1]);
            d->set_length(1);
            d->set_realval(0, (double)data.v.iValue);
        }
    }
    else if (dtype == IF_REAL) {
        if (data.type & IF_VECTOR) {
            d->set_length(data.v.v.numValue);
            d->set_realvec(new double[d->length()]);
            memcpy(d->realvec(), data.v.v.vec.rVec,
                d->length()*sizeof(double));
        }
        else {
            d->set_realvec(new double[1]);
            d->set_length(1);
            d->set_realval(0, data.v.rValue);
        }
    }
    else if (dtype == IF_COMPLEX) {
        d->set_flags(d->flags() | VF_COMPLEX);
        if (data.type & IF_VECTOR) {
            d->set_length(data.v.v.numValue);
            d->set_compvec(new complex[d->length()]);
            for (int j = 0; j < d->length(); j++) {
                d->set_realval(j, data.v.v.vec.cVec[j].real);
                d->set_imagval(j, data.v.v.vec.cVec[j].imag);
            }
        }
        else {
            d->set_compvec(new complex[1]);
            d->set_length(1);
            d->set_realval(0, data.v.cValue.real);
            d->set_imagval(0, data.v.cValue.imag);
        }
    }
    else if (dtype == IF_PARSETREE) {
        d->set_length(1);
        d->set_realvec(new double[1]);
        d->set_realval(0, 0.0);

        IFparseTree *tree = data.v.tValue;
        if (tree) {
            int numvars = tree->num_vars();
            double *vals = new double[numvars + 1];
            for (int i = 0; i < numvars; i++) {
                sLstr lstr;
                tree->varName(i, lstr);
                sDataVec *v = vecGet(lstr.string(), ckt, silent);
                if (!v) {
                    delete [] vals;
                    return (0);
                }
                vals[i] = v->realval(0);
            }
            double r;
            int err = tree->eval(&r, vals, 0);
            delete [] vals;
            if (err != OK)
                return (0);
            d->set_realval(0, r);
        }
    }
    else if (dtype == IF_STRING) {
        // Sort of a hack, use the datavec to pass back a string.
        d->set_length(1);
        d->set_realvec(new double[1]);
        d->set_realval(0, 0.0);
        d->set_defcolor(data.v.sValue);
        d->set_flags(d->flags() | VF_STRING);
    }
    else {
        if (!silent)
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "non-numeric data type for %s.\n", word);
        return (0);
    }
    return (d);
}


// Same logic as vecGet, but just return true if word contains a
// string resolvable as a vector name in the current context.
//
bool
IFoutput::isVec(const char *word, const sCKT *ckt)
{
    sPlot *pl = curPlot();
    sDataVec *d = pl->find_vec(word);
    if (d)
        return (true);

    bool allflag;
    const char *tw = word;
    pl = stripplot(&word, &allflag);
    if (pl) {
        d = pl->find_vec(word);
        if (d)
            return (d);
        word = tw;
        pl = curPlot();
    }
    else if (allflag)
        return (true);
    else
        pl = curPlot();

    if (cxPlot() && cxPlot() != pl && cxPlot() != o_constants)
        d = cxPlot()->find_vec(word);
    if (!d)
        d = o_constants->find_vec(word);
    if (d)
        return (true);

    char *name = lstring::copy(word);
    char *param;
    for (param = name; *param && (*param != '['); param++) ;
    if (*param) {
        *param++ = '\0';
        char *s;
        for (s = param; *s && *s != ']'; s++) ;
        if (*s == ']') {
            const char *resid = s+1;
            *s = '\0';

            // Don't recognize the vector if there is something
            // following, i.e., it is part of an expression.
            while (isspace(*resid))
                resid++;
            if (*resid)
                return (false);
        }
    }
    else
        param = 0;
    IFdata data;
    if (*word == Sp.SpecCatchar()) {
        // This is a special quantity...
        int err = 1;
        if (param && *param) {
            if (ckt) {
                err = ckt->getParam(name+1, param, &data);
                if (err)
                    err = ckt->getAnalParam(name+1, param, &data);
            }
        }
        else {
            if (ckt) {
                err = OPTinfo.getOpt(ckt, 0, name+1, &data);
                if (err)
                    err = ckt->CKTbackPtr->dotparam(name+1, &data);
                if (err)
                    err = STATinfo.getOpt(ckt, 0, name+1, &data);
            }
            if (err) {
                if (lstring::eq(name+1, kw_elapsed))
                    err = 0;
                else if (lstring::eq(name+1, kw_space))
                    err = 0;
            }
        }
        delete [] name;
        return (!err);
    }

    bool ok = (ckt && ckt->CKTbackPtr->getVerilog(name, param, &data));
    // If ok is true, name was resolved in Verilog block.
    delete [] name;

    if (ok)
        return (true);

    // Look for some global names.  The only one supported
    // so far is "temper" which returns the current
    // temperature in Celsius, same as HSPICE.

    if (lstring::eq(word, "temper"))
        return (true);

    return (false);
}


namespace {
    // Convenience function for evaluation.  Note that the context is that
    // of the current plot.
    //
    sDataVec *evaluate(const char **wordp)
    {
        const char *wrd = *wordp;
        pnode *nn = Sp.GetPnode(wordp, true);
        if (!nn) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "evaluation failed: %s.\n", wrd);
            return (0);
        }
        sDataVec *t = Sp.Evaluate(nn);
        delete nn;
        if (!t)
            GRpkgIf()->ErrPrintf(ET_ERROR, "evaluation failed: %s.\n", wrd);
        return (t);
    }
}


// The arg is a string of the form "vector = expression".  This evaluates
// the expression and makes assignments.
//
// If rdonly is true, the new vector will be read-only.
//
// If rhsend is given, it will contain a pointer into rhs to the first
// char following the expression.
//
void
IFoutput::vecSet(const char *lhs, const char *rhs, bool rdonly,
    const char **rhsend)
{
    if (!lhs || !rhs)
        return;
    if (rhsend)
        *rhsend = rhs;

    // sanity check
    if (lstring::cieq(lhs, "all") || lstring::ciprefix("all.", lhs)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't set \"all\".\n");
        return;
    }
    if (*lhs == Sp.SpecCatchar()) {
        // Specials, at least @device[param], can be set now.
        // GRpkgIf()->ErrPrintf(ET_ERROR, "%s is read-only.\n", lhs);
        // return;

        char *name = lstring::copy(lhs);
        char *param;
        for (param = name; *param && (*param != '['); param++) ;
        if (*param) {
            *param++ = '\0';
            char *s;
            for (s = param; *s && *s != ']'; s++) ;
            *s = '\0';
        }
        else
            param = 0;
        if (!*(name+1)) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "no device or model name given for %s.\n", lhs);
            delete [] name;
            return;
        }
        if (!param || !*param) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "no parameter name given for %s.\n", lhs);
            delete [] name;
            return;
        }
        if (!Sp.CurCircuit()) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "can't set parameter, no current circuit.\n");
            delete [] name;
            return;
        }
        if (rhsend) {
            const char *trhs = rhs;
            sDataVec *t = evaluate(&trhs);
            if (!t) {
                delete [] name;
                return;
            }
            if (rhsend)
                *rhsend = trhs;
        }
        Sp.CurCircuit()->addDeferred(name+1, param, rhs);

        delete [] name;
        return;
    }

    // Since we may write into the lhs string...
    char lbuf[256];
    strcpy(lbuf, lhs);
    lhs = lbuf;
    const char *r = strchr(lhs, '[');
    char *range = 0;
    if (r) {
        range = lbuf + (r - lhs);
        *range++ = 0;
    }

    // A plot.name specification will force the new vector to be created
    // in plot.
    //
    bool allflag;
    bool plgiven = false;
    sPlot *pl = stripplot(&lhs, &allflag);
    if (!pl) {
        if (allflag) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "\"all\" not valid in this context.\n");
            return;
        }
        pl = curPlot();
    }
    else
        plgiven = true;

    // extract indices
    int indices[MAXDIMS];
    int numdims = 0;
    char *s = range;
    if (s) {
        int need_open = 0;
        while (!need_open || *s == '[') {
            if (need_open)
                s++;
            int depth = 0;
            char *q;
            for (q = s; *q && ((*q != ']' && *q != ',') || depth > 0); q++) {
                if (*q == '[')
                    depth++;
                else if (*q == ']')
                    depth--;
            }

            if (depth != 0 || !*q) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "bad syntax specifying index.\n");
                return;
            }

            if (*q == ']')
                need_open = 1;
            else
                need_open = 0;
            if (*q)
                *q++ = 0;

            // evaluate expression between s and q
            const char *stmp = s;
            sDataVec *t = evaluate(&stmp);
            if (!t)
                return;

            if (!t->isreal() || t->link() || t->length() != 1 ||
                    !t->realvec()) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "index is not a scalar.\n");
                return;
            }

            int j = (int)t->realval(0); // ignore sanity checks for now
            if (j < 0) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "negative index (%d) is not allowed.\n", j);
                return;
            }
            indices[numdims++] = j;

            for (s = q; *s && isspace(*s); s++) ;
        }
    }

    // evaluate rhs
    sDataVec *t = evaluate(&rhs);
    if (!t)
        return;
    if (rhsend)
        *rhsend = rhs;

    if (t->link()) {
        GRpkgIf()->ErrPrintf(ET_WARN, "extra wildcard values ignored.\n");
        t = t->link()->dl_dvec;
    }

    // make sure dims are ok for unit dimension
    if (t->numdims() <= 1) {
        t->set_numdims(1);
        t->set_dims(0, t->length());
    }
    if (numdims + t->numdims() > MAXDIMS) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "too many dimensions.\n");
        return;
    }

    sDataVec *n;
    if (plgiven)
        n = pl->find_vec(lhs);
    else
        n = vecGet(lhs, 0);
    if (n && (n->flags() & VF_READONLY)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "specified vector %s is read-only.\n",
            lhs);
        return;
    }
    if (!n) {

        // Create and assign a new vector. If a range was specified,
        // create a vector of the dimensionality and size to contain
        // the data.  The least significant dimension given is added
        // to the most significant dimension of the vector being assigned.
        // The entries not explicitly assigned are zero. E.g.,
        // let a[2] = b.  If b is a 1d vector of length n, a will be a
        // 1d vector of length n+2 with entries (0, 0, b[0], b[1], ...).
        // If b has dimensions [x][y][z], a will have dimensions
        // [x+2][y][z].  for let a[2][3] = b, a will have dimensions
        // [2+1][x+3][y][z], etc.
        //
        n = new sDataVec(lstring::copy(lhs), t->flags(), 0, t->units());
        if (rdonly)
            n->set_flags(n->flags() | VF_READONLY);
        n->set_defcolor(t->defcolor());
        n->set_gridtype(t->gridtype());
        n->set_plottype(t->plottype());
        n->newperm(pl);

        // set the scale, make sure it is permanent, too
        if (t->scale()) {
            if (pl->get_perm_vec(t->scale()->name())) {
                // Note: this may point to a dvec in a different plot,
                // if there is a name clash in pl.
                n->set_scale(t->scale());
            }
            else {
                n->set_scale(t->scale()->copy());
                n->scale()->newperm(pl);
            }
        }

        int i = 0;
        if (numdims) {
            for ( ; i < numdims - 1; i++)
                n->set_dims(i, indices[i] + 1);
            n->set_dims(numdims-1, indices[numdims-1]);
        }
        if (t->numdims() <= 1) {
            n->set_dims(i, n->dims(i) + t->length());
            n->set_numdims(numdims ? numdims : 1);
        }
        else {
            n->set_dims(i, n->dims(i) + t->dims(0));
            int j;
            for (i++, j = 1; j < t->numdims(); i++, j++)
                n->set_dims(i, t->dims(j));
            n->set_numdims(i);
        }

        int size = 1;
        for (i = 0; i < n->numdims(); i++)
            size *= n->dims(i);
        int rsize = 1;
        for (i = 0; i < t->numdims(); i++)
            rsize *= t->dims(i);
        // rsize should be t->length(), but maybe not...
        int offset = size - rsize;

        if (offset < 0 || rsize < t->length()) {
            GRpkgIf()->ErrPrintf(ET_INTERR, "bad vector length.\n");
            pl->remove_vec(n->name());
        }
        else {
            n->alloc(t->isreal(), size);
            t->copyto(n, 0, offset, t->length());
            n->set_length(size);
        }
        if (pl == curPlot())
            ToolBar()->UpdateVectors(0);
        return;
    }
    if (numdims == 0) {
        // reuse vector n, copy stuff from t
        *n->units() = *t->units();
        n->set_flags(t->flags());
        if (rdonly)
            n->set_flags(n->flags() | VF_READONLY);
        n->realloc(t->length());
        t->copyto(n, 0, 0, t->length());

        // set the scale, make sure it is permanent, too
        if (t->scale()) {
            if (pl->get_perm_vec(t->scale()->name())) {
                // Note: this may point to a dvec in a different plot,
                // if there is a name clash in pl.
                n->set_scale(t->scale());
            }
            else {
                n->set_scale(t->scale()->copy());
                n->scale()->newperm(pl);
            }
        }
        n->set_numdims(t->numdims());
        for (int i = 0; i < t->numdims(); i++)
            n->set_dims(i, t->dims(i));
        n->set_length(t->length());
        if (pl == curPlot())
            ToolBar()->UpdateVectors(0);
        return;
    }

    // make sure dims are ok for unit dimension
    if (n->numdims() <= 1) {
        n->set_numdims(1);
        n->set_dims(0, n->length());
    }

    // Here we copy into an existing vector, expanding the size if
    // necessary. The given indices except the least significant must
    // be within the range of n, and "point" to a submatrix with the
    // same dimensionality as t.  However, if t is a 1-d vector,
    // t is copied into n at the index, if it will fit, but if n is
    // a 1-d vector n is extended if necessary. Values not explicitly
    // set remain as they were, and new values created are zero.
    //

    if (numdims > n->numdims() || t->numdims() > n->numdims()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't set %s to higher dimension.\n",
            n->name());
        return;
    }
    for (int i = 0; i < numdims-1; i++) {
        if (indices[i] >= n->dims(i)) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "dimension spec. out of range for %s.\n", n->name());
            return;
        }
    }
    if (t->numdims() > 1) {
        int i, j;
        for (i = numdims, j = 1; j < t->numdims(); i++,j++) {
            if (i >= n->numdims() || n->dims(i) != t->dims(j)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, 
                    "subdimension of %s is incompatible with %s.\n",
                    n->name(), t->name());
                return;
            }
        }
    }
    else if (n->numdims() > 1) {
        if (n->numdims() - numdims > 1) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "too few dimensions specified.\n");
            return;
        }

        // will t fit in block?
        int i;
        if (n->numdims() == numdims)
            i = indices[numdims-1];
        else
            i = 0;

        if (t->dims(0) + i > n->dims(n->numdims()-1)) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "operation would overflow block in %s.\n", n->name());
            return;
        }
    }

    int offset = 0;
    int size = 1;
    int i;
    for (i = n->numdims()-1; i >= 0; i--) {
        if (i < numdims)
            offset += indices[i]*size;
        size *= n->dims(i);
    }
    int rsize = 1;
    for (i = 0; i < t->numdims(); i++)
        rsize *= t->dims(i);
    // rsize should be t->length(), but maybe not...
    if (rsize < t->length()) {
        GRpkgIf()->ErrPrintf(ET_INTERR, "bad vector length #2.\n");
        return;
    }

    // fix the dimension
    int j = numdims-1;
    i = indices[j];
    if (n->numdims() > t->numdims())
        i++;
    else
        i += t->dims(0);
    if (n->dims(j) < i)
        n->set_dims(j, i);

    if (offset + rsize > n->length()) {
        // have to allocate more space
        n->set_length(offset + rsize);
        n->resize(n->length());
    }
    t->copyto(n, 0, offset, t->length());
    if (pl == curPlot())
        ToolBar()->UpdateVectors(0);
}


// Clear out the temporary vectors linked into the plot.  If purge_temp
// is true, delete the vectors with the VF_TEMPORARY flag set.  We can
// use this to more locally control vector purging:
//
//   sDataVec::set_temporary(true);
//    ...
//   sDataVec::set_temporary(false);
//   OP.vecGc(true);
//
void
IFoutput::vecGc(bool purge_temp)
{
    for (sPlot *pl = plotList(); pl; pl = pl->next_plot()) {
        sDataVec *d, *nd, *pd = 0;
        for (d = pl->tempvecs(); d; d = nd) {
            nd = d->next();

            if (!purge_temp || (d->flags() & VF_TEMPORARY)) {
                if (Sp.GetFlag(FT_VECDB)) {
                    GRpkgIf()->ErrPrintf(ET_MSGS,
                        "VecGc: throwing away %s.%s\n",
                        pl->type_name(), d->name());
                }
                if (pl->scale() == d) {
                    // this shouldn't happen
                    pl->set_scale(0);
                    wordlist *wl = pl->list_perm_vecs();
                    if (wl) {
                        pl->set_scale(pl->get_perm_vec(wl->wl_word));
                        wordlist::destroy(wl);
                    }
                }
                if (!pd)
                    pl->set_tempvecs(nd);
                else
                    pd->set_next(nd);
                delete d;
                continue;
            }
            pd = d;
        }
    }
}


// Static function.
// This is something we do in a few places...  Since vectors get copied a
// lot, we can't just compare pointers to tell if two vectors are 'really'
// the same.
//
bool
IFoutput::vecEq(sDataVec *v1, sDataVec *v2)
{
    if (v1->plot() != v2->plot())
        return (false);
    
    const char *s1 = v1->basename();
    const char *s2 = v2->basename();

    int i = lstring::cieq(s1, s2);
    delete [] s1;
    delete [] s2;

    return (i);
}


// Display vector status, etc.  Note that this only displays stuff
// from the current plot, and you must do a setplot to see the rest of
// it.  If retstr is not 0, the info is returned appended to *retstr,
// otherwise it is printed to standard output.
//
void
IFoutput::vecPrintList(wordlist *wl, char **retstr)
{
    char buf[BSIZE_SP];
    while (wl) {
        char *s = lstring::copy(wl->wl_word);
        CP.Unquote(s);
        sCKT *ckt = Sp.CurCircuit() ? Sp.CurCircuit()->runckt() : 0;
        sDataVec *d = vecGet(s, ckt);
        delete [] s;
        if (d == 0)
            Sp.Error(E_NOVEC, 0, wl->wl_word);
        else {
            if (d->link()) {
                for (sDvList *dl = d->link(); dl; dl = dl->dl_next)
                    dl->dl_dvec->print(retstr);
            }
            else
                d->print(retstr);
        }
        if (wl->wl_next == 0)
            return;
        wl = wl->wl_next;
    }
    if (!curPlot()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no current plot.\n");
        return;
    }

    wordlist *tl0 = curPlot()->list_perm_vecs();
    if (!retstr)
        TTY.init_more();
    if (!tl0) {
        const char *s = "There are no vectors currently active.\n";
        if (retstr)
            *retstr = lstring::build_str(*retstr, s);
        else
            TTY.printf(s);
        return;
    }
    if (!Sp.GetVar(kw_nosort, VTYP_BOOL, 0))
        wordlist::sort(tl0);
    sprintf(buf, "Title: %s\n",  curPlot()->title());
    sprintf(buf + strlen(buf), "Name: %s (%s)\nDate: %s\n\n", 
        curPlot()->type_name(), curPlot()->name(), curPlot()->date());
    if (retstr)
        *retstr = lstring::build_str(*retstr, buf);
    else
        TTY.send(buf);

    // list scale first
    if (!curPlot()->scale()) {
        for (wordlist *tl = tl0; tl; tl = tl->wl_next) {
            sDataVec *d = curPlot()->get_perm_vec(tl->wl_word);
            d->print(retstr);
        }
    }
    else {
        for (wordlist *tl = tl0; tl; tl = tl->wl_next) {
            if (lstring::eq(tl->wl_word, curPlot()->scale()->name())) {
                sDataVec *d = curPlot()->get_perm_vec(tl->wl_word);
                d->print(retstr);
                break;
            }
        }
        for (wordlist *tl = tl0; tl; tl = tl->wl_next) {
            if (!lstring::eq(tl->wl_word, curPlot()->scale()->name())) {
                sDataVec *d = curPlot()->get_perm_vec(tl->wl_word);
                d->print(retstr);
            }
        }
    }
    wordlist::destroy(tl0);
}
// End of IFoutput functions

