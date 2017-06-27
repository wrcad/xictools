
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: vectors.cc,v 2.122 2017/02/22 01:53:07 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "rawfile.h"
#include "csdffile.h"
#include "prntfile.h"
#include "fteparse.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "commands.h"
#include "toolbar.h"
#include "circuit.h"
#include "optdefs.h"
#include "statdefs.h"
#include "errors.h"
#include "hash.h"
#include "spnumber.h"


//
// Functions for dealing with the vector database.
//

namespace {
    sDataVec *evaluate(const char**);
    sPlot *stripplot(const char**, bool*);

    // Where 'constants' go when defined on initialization.
    //
    struct sConstPlot : public sPlot
    {
        sConstPlot() : sPlot(0) {
            set_title("Constant values");
            set_date(datestring());
            set_name("constants");
            set_type_name("constants");
            set_written(true);
        }
    };
    sConstPlot constplot;
}

// Export the "constants" plot.
sPlot *sPlot::pl_constants = &constplot;


// This can now handle multiple assignments on one line:
// let a = b c = d ...
//
void
CommandTab::com_let(wordlist *wl)
{
    if (!wl) {
        Sp.VecPrintList(0, 0);
        return;
    }
    char *p = wl->flatten();

    char *rhs = strchr(p, '=');
    if (!rhs) {
        Sp.VecPrintList(wl, 0);
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
        Sp.VecSet(lhs, rhs, false, &rhs_residue);
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
            for (pl = Sp.PlotList(); pl; pl = pl->next_plot())
                pl->remove_vec(s);
        }
        else
            Sp.CurPlot()->remove_vec(s);
    }
    ToolBar()->UpdateVectors(1);
}


// Display vector status, etc.  Note that this only displays stuff from the
// current plot, and you must do a setplot to see the rest of it.
//
void
CommandTab::com_display(wordlist *wl)
{
    Sp.VecPrintList(wl, 0);
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
            pl0->free();
            dl0->free();
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
    pl0->free();

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
    dl0->free();
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
            pl0->free();
            dl0->free();
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
    pl0->free();

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
    dl0->free();
    ToolBar()->UpdateVectors(0);
}


// Set the current working plot.
//
void
CommandTab::com_setplot(wordlist *wl)
{
    const char *zz = CP.GetFlag(CP_NOTTYIO) ?
        "Plots in memory, run command again with desired name:" :
        "Type the name of the desired plot: ";
    char buf[BSIZE_SP];
    if (wl == 0) {
        TTY.printf("        new        New plot\n");
        for (sPlot *p = Sp.PlotList(); p; p = p->next_plot()) {
            if (Sp.CurPlot() == p)
                TTY.printf("Current %-11s%-20s (%s)\n",
                p->type_name(), p->title(), p->name());
            else
                TTY.printf("        %-11s%-20s (%s)\n",
                    p->type_name(), p->title(), p->name());
        }
        TTY.send("\n");
        if (CP.GetFlag(CP_NOTTYIO))
            return;
        TTY.prompt_for_input(buf, BSIZE_SP, zz);

        char *s;
        for (s = buf; *s && !isspace(*s); s++) ;
        *s = '\0';
    }
    else
        strcpy(buf, wl->wl_word);

    if (!*buf)
        return;
    Sp.SetCurPlot(buf);
    if (wl)
        TTY.printf("%s %s (%s)\n", Sp.CurPlot()->type_name(),
            Sp.CurPlot()->title(), Sp.CurPlot()->name());
    if (Sp.CurPlot()->circuit()) {
        Sp.SetCircuit(Sp.CurPlot()->circuit());
        Sp.OptUpdate();
        TTY.printf("current circuit: %s, %s\n", Sp.CurCircuit()->name(),
            Sp.CurCircuit()->descr());
    }
}


void
CommandTab::com_destroy(wordlist *wl)
{
    if (!wl)
        Sp.RemovePlot(Sp.CurPlot());
    else if (lstring::cieq(wl->wl_word, "all"))
        Sp.RemovePlot("all");
    else {
        ToolBar()->UpdatePlots(1);
        while (wl) {
            sPlot *pl;
            for (pl = Sp.PlotList(); pl; pl = pl->next_plot())
                if (lstring::eq(pl->type_name(), wl->wl_word))
                    break;
            if (pl)
                Sp.RemovePlot(pl);
            else
                GRpkgIf()->ErrPrintf(ET_ERROR, "no such plot %s.\n",
                    wl->wl_word);
            wl = wl->wl_next;
        }
        ToolBar()->UpdatePlots(1);
    }
}


// Command to ombine the last two "similar" plots into a single
// multi-dimensional plot
//
void
CommandTab::com_combine(wordlist*)
{
    if (!Sp.PlotList()) {
        GRpkgIf()->ErrPrintf(ET_INTERR, "no plots in list!\n");
        return;
    }
    if (Sp.PlotList() == sPlot::constants()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't combine constants plot.\n");
        return;
    }
    if (!Sp.PlotList()->next_plot()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no plot to combine.\n");
        return;
    }
    if (Sp.PlotList()->next_plot() == sPlot::constants()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't combine constants plot.\n");
        return;
    }
    if (Sp.PlotList()->next_plot()->add_segment(Sp.PlotList()))
        Sp.PlotList()->destroy();
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "incompatible plots, can't combine.\n");
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
        sDataVec *v = Sp.CurPlot()->find_vec(wl->wl_word);
        if (v) {
            Sp.CurPlot()->set_scale(v);
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
        sPlot *pl = Sp.FindPlot(wl->wl_word);
        if (pl) {
            sDataVec *v = pl->find_vec(sl->wl_word);
            if (v) {
                pl->set_scale(v);
                if (pl == Sp.CurPlot())
                    ToolBar()->UpdateVectors(0);
                return;
            }
            GRpkgIf()->ErrPrintf(ET_ERROR, errm, sl->wl_word);
            return;
        }
    }
    sPlot *pl = Sp.CurPlot();
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
    if (pl == Sp.CurPlot())
        ToolBar()->UpdateVectors(0);
}


void
CommandTab::com_setdim(wordlist *wl)
{
    // setdim numdims dim0 dim1 ...
    // numdims-1 dims given

    sPlot *p = Sp.CurPlot();
    if (!p) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no current plot!");
        return;
    }

    if (!p->scale()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "current plot has no scale.");
        return;
    }
    int len = p->scale()->length();
    if (len <= 1) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "current plot has no dimensionality.");
        return;
    }

    if (!wl) {
        sDataVec *sc = p->scale();
        int numdims = sc->numdims();
        TTY.printf("length: %d", len);
        TTY.printf("  numdims: %d", numdims);
        if (numdims >= 2) {
            TTY.printf("  dims:");
            for (int i = 0; i < numdims - 1; i++)
                TTY.printf(" %d", sc->dims(i));
        }
        TTY.printf("\n");
        return;
    }

    int cnt = 0;
    int numdims = 0;
    int dims[MAXDIMS];
    memset(dims, 0, MAXDIMS*sizeof(int));
    for (wordlist *w = wl; w; w = w->wl_next) {
        int n;
        if (sscanf(w->wl_word, "%d", &n) < 1 || n < 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "bad input, non-negative integer expected.");
            return;
        }
        if (cnt == 0) {
            if (n == 0)
                n = 1;
            if (n == len) {
                // Special case: each value is a dimension.
                dims[0] = n;
                dims[1] = 1;
                p->set_dims(2, dims);
                return;
            }
            if (n > MAXDIMS) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "too many dimensions given, maximum %d.", MAXDIMS);
                return;
            }
            numdims = n;
        }
        else {
            if (n < 2) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "bad dimension, must be two or larger.");
                return;
            }
            dims[cnt-1] = n;
        }
        cnt++;
    }
    if (cnt < numdims) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "too few values given.");
        return;
    }
    else if (cnt > numdims)
        GRpkgIf()->ErrPrintf(ET_WARN, "extra values given, ignored.");

    int n = 1;
    for (int i = 0; i < numdims - 1; i++)
        n *= dims[i];
    dims[numdims - 1] = len/n;

    p->set_dims(numdims, dims);
}
// End of CommandTab functions.


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
            sPlot *p = Sp.FindPlot(bf);
            delete [] bf;
            if (p) {
                *vname = ++s;
                return (p);
            }
        }
        return (0);
    }
}


// Find a plot by name.  The name can be the type_name, or an
// absolute or relative index into the plot list.  If + or -
// followed by a number, the plot is relative to the current plot
// in the plot list.  If a number, it is the index in the plot
// list, 0-based.
//
// Also recognize new forms that don't confuse the math parser.
//  prevN  equiv to -N, N optional defaults to 1
//  nextN  equiv to +N, N optional defaults to 1
//  plotN  equiv to N, N optional defaults to 0
//
sPlot *
IFsimulator::FindPlot(const char *name)
{
    if (name[0] == '-' && isdigit(name[1])) {
        int n = atoi(name+1);
        if (n < 1)
            return (CurPlot());
        for (sPlot *pl = CurPlot(); pl; pl = pl->next_plot()) {
            if (!n--)
                return (pl);
        }
    }
    else if (!strncmp(name, "prev", 4) && (!name[4] || isdigit(name[4]))) {
        // New form "prev[N]" which avoids the math ambiguity. 
        // The square brackets above are not literal, N is an
        // integer, one if not given.

        int n = 1;
        if (isdigit(name[4]))
            n = atoi(name+4);
        if (!n)
            return (CurPlot());
        for (sPlot *pl = CurPlot(); pl; pl = pl->next_plot()) {
            if (!n--)
                return (pl);
        }
    }
    else if (name[0] == '+' && isdigit(name[1])) {
        int n = atoi(name+1);
        if (n < 1)
            return (CurPlot());
        int c = 0;
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot()) {
            if (pl == CurPlot())
                break;
            c++;
        }
        if (c < n)
            return (0);
        c -= n;
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot()) {
            if (!c--)
                return (pl);
        }
    }
    else if (!strncmp(name, "next", 4) && (!name[4] || isdigit(name[4]))) {
        // New form "next[N]" which avoids the math ambiguity. 
        // The square brackets above are not literal, N is an
        // integer, one if not given.

        int n = 1;
        if (isdigit(name[4]))
            n = atoi(name+4);
        if (!n)
            return (CurPlot());
        int c = 0;
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot()) {
            if (pl == CurPlot())
                break;
            c++;
        }
        if (c < n)
            return (0);
        c -= n;
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot()) {
            if (!c--)
                return (pl);
        }
    }
    else if (isdigit(*name)) {
        int c = 0;
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot())
            c++;
        c--;
        int n = atoi(name);
        if (c < n)
            return (0);
        c -= n;
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot()) {
            if (!c--)
                return (pl);
        }
    }
    else if (!strncmp(name, "plot", 4) && (!name[4] || isdigit(name[4]))) {
        // New form "plot[N]" which avoids the math ambiguity. 
        // The square brackets above are not literal, N is an
        // integer, zero if not given.

        int n = 0;
        if (isdigit(name[4]))
            n = atoi(name+4);
        int c = 0;
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot())
            c++;
        c--;
        if (c < n)
            return (0);
        c -= n;
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot()) {
            if (!c--)
                return (pl);
        }
    }
    else if (!strcmp(name, "curplot")) {
        return (CurPlot());
    }
    else {
        for (sPlot *pl = PlotList(); pl; pl = pl->next_plot()) {
            if (PlotPrefix(name, pl->type_name()))
                return (pl);
        }
    }
    return (0);
}


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
IFsimulator::VecGet(const char *word, const sCKT *ckt, bool silent)
{
    if (!word || !*word) {
        if (!silent) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "VecGet, null or empty vector name encountered.\n");
        }
        return (0);
    }
    sPlot *pl = ft_plot_cur;
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
        pl = ft_plot_cur;
    }
    else if (allflag) {
        // plot is wildcard
        sDvList *dl0 = 0, *dl = 0;
        for (pl = ft_plot_list; pl; pl = pl->next_plot()) {
            if (pl == sPlot::constants())
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
        pl = ft_plot_cur;

    if (ft_plot_cx && ft_plot_cx != pl && ft_plot_cx != sPlot::constants())
        d = ft_plot_cx->find_vec(word);
    if (!d)
        d = sPlot::constants()->find_vec(word);
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
    if (*word == SpecCatchar()) {
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

        IFparseTree *tree = data.v.tValue;
        int numvars = tree->num_vars();
        double *vals = new double[numvars + 1];
        for (int i = 0; i < numvars; i++) {
            sLstr lstr;
            tree->varName(i, lstr);
            sDataVec *v = VecGet(lstr.string(), ckt, silent);
            if (!v) {
                delete [] vals;
                return (0);
            }
            vals[i] = v->realval(0);
        }
        double r;
        int error = tree->eval(&r, vals, 0);
        delete [] vals;
        if (error)
            return (0);
        d->set_realval(0, r);
    }
    else {
        if (!silent)
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "non-numeric data type for %s.\n", word);
        return (0);
    }
    return (d);
}


// Same logic as VecGet, but just return true if word contains a
// string resolvable as a vector name in the current context.
//
bool
IFsimulator::IsVec(const char *word, const sCKT *ckt)
{
    sPlot *pl = ft_plot_cur;
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
        pl = ft_plot_cur;
    }
    else if (allflag)
        return (true);
    else
        pl = ft_plot_cur;

    if (ft_plot_cx && ft_plot_cx != pl && ft_plot_cx != sPlot::constants())
        d = ft_plot_cx->find_vec(word);
    if (!d)
        d = sPlot::constants()->find_vec(word);
    if (d)
        return (true);

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
    if (*word == SpecCatchar()) {
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


// The arg is a string of the form "vector = expression".  This evaluates
// the expression and makes assignments.
//
// If rdonly is true, the new vector will be read-only.
//
// If rhsend is given, it will contain a pointer into rhs to the first
// char following the expression.
//
void
IFsimulator::VecSet(const char *lhs, const char *rhs, bool rdonly,
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
    if (*lhs == SpecCatchar()) {
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
        if (!ft_curckt) {
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
        ft_curckt->addDeferred(name+1, param, rhs);

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
        pl = ft_plot_cur;
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
        n = VecGet(lhs, 0);
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
        if (pl == ft_plot_cur)
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
        if (pl == ft_plot_cur)
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
    if (pl == ft_plot_cur)
        ToolBar()->UpdateVectors(0);
}

namespace {
    bool parse_coltok(const char *str, int *ncols, int *xcols)
    {
        if (!str)
            return (false);
        if (str[0] != '-' || str[1] != 'c')
            return (false);
        const char *pp = 0;
        for (const char *s = str+2; *s; s++) {
            if (!isdigit(*s) && *s != '+')
                return (false);
            if (*s == '+')
                pp = s;
        }
        if (!pp) {
            *ncols = atoi(str+2);
            *xcols = -1;
            return (true);
        }
        *ncols = atoi(str+2);
        pp++;
        *xcols = (isdigit(*pp) ? atoi(pp) : 0);
        return (true);
    }
}


// Load in a data file.  One or more plots will be created, the
// written flag will be set in new plots if the written argument is
// true.
//
// The fnameptr points to a list of file names and directives, and is
// updated upon return.  Only one file is read per call.
// The recognized tokens are:
//   1. The name of a rawfile or CSDF file.
//   2. -p printfile, where printfile was generated with the print command
//      in column format.
//   3. -cN[[+]M] datafile, where datafile contains columns of numbers.
//      -cN   : save N numbers from lines that have at least N numbers.
//      -cN+  : save N numbers from lines that have exactly N numbers.
//      -cN+M : The assumption here is that M-number blocks are written
//              after the N-number blocks, so that lines in the file are
//              not too long, but logically we have N+M numbers.
//              It is required that M < N.
//
void
IFsimulator::LoadFile(const char **fnameptr, bool written)
{
    char *file = lstring::getqtok(fnameptr);
    if (!file)
        return;
    bool printfmt = false;
    int ncols = 0, xcols = -1;
    if (!strcmp(file, "-p")) {
        printfmt = true;
        delete [] file;
        file = lstring::getqtok(fnameptr);
        if (!file)
            return;
    }
    else if (parse_coltok(file, &ncols, &xcols)) {
        delete [] file;
        if (!ncols)
            return;
        file = lstring::getqtok(fnameptr);
        if (!file)
            return;
    }
    GCarray<char*> gc_file(file);

    bool is_csdf = false;
    if (!printfmt && !ncols) {
        FILE *fp = Sp.PathOpen(file, "rb");
        if (!fp) {
            GRpkgIf()->Perror(file);
            TTY.printf("Warning: no data read.\n");
            return;
        }

        // Check the file opening chars for a CSDF header record
        // prefix.
        //
        char buf[32];
        if (fread(buf, 1, 32, fp) != 32) {
            fclose(fp);
            TTY.printf("Warning: bad format, no data read.\n");
            return;
        }
        for (int i = 0; i < 31; i++) {
            if (isspace(buf[i]))
                continue;
            if (buf[i] == '#' && buf[i+1] == 'H')
                is_csdf = true;
            break;
        }
        fclose(fp);
    }

    sPlot *pl;
    if (printfmt) {
        TTY.printf("Loading print data file (\"%s\") . . . ", file);
        cPrintIn in;
        pl = in.read(file);
        if (pl)
            TTY.printf("done.\n");
        else {
            TTY.printf("Warning: no data read.\n");
            return;
        }
    }
    else if (ncols) {
        TTY.printf("Loading column data file (\"%s\") . . . ", file);
        cColIn in;
        pl = in.read(file, ncols, xcols);
        if (pl)
            TTY.printf("done.\n");
        else {
            TTY.printf("Warning: no data read.\n");
            return;
        }
    }
    else if (is_csdf) {
        TTY.printf("Loading CSDF data file (\"%s\") . . . ", file);
        cCSDFin csdf;
        pl = csdf.csdf_read(file);
        if (pl)
            TTY.printf("done.\n");
        else {
            TTY.printf("Warning: no data read.\n");
            return;
        }
    }
    else {
        TTY.printf("Loading raw data file (\"%s\") . . . ", file);
        cRawIn raw;
        pl = raw.raw_read(file);
        if (pl)
            TTY.printf("done.\n");
        else {
            TTY.printf("Warning: no data read.\n");
            return;
        }
    }

    sPlot *np;
    for (; pl; pl = np) {
        np = pl->next_plot();
        pl->add_plot();
        // If not from temp file...
        if (written)
            pl->set_written(true);
    }
    ToolBar()->UpdatePlots(1);
}


// Make a plot the current one.  This gets called by cp_usrset() when one
// does a 'set curplot = name'.
//
void
IFsimulator::SetCurPlot(const char *name)
{
    if (lstring::cieq(name, "new")) {
        sPlot *pl = new sPlot("unknown");
        pl->new_plot();
        ft_plot_cur = pl;
    }
    else {
        sPlot *pl = FindPlot(name);
        if (!pl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "can't find plot named %s.\n",
                name);
            return;
        }
        ft_plot_cur = pl;
        pl->run_commands();
    }
    ToolBar()->UpdatePlots(0);
}


// Consider the following script:
// 
//     title
//     .control
//     let x = 1
//     while (x < 5)
//         source circuit.cir
//         run
//         print x
//         x = x + 1
//     end
// 
// Previous behavior
// 
// This will only work when started with the current plot being the
// constants plot.  The vector x is defined in the constants plot.  The
// run changes the current plot, but x is still resolved into the
// constants plot, since the constants plot is always searched.  Once a
// vector is defined in the constants plot, it can not be removed, so
// repeated runs of the script will always work.
// 
// However, if the initial plot was not the constants plot, x would be
// defined in that plot, and the script would fail when x is accessed
// after the run.
//
// Current behavior
// 
// Now, before executing a script, the current plot is saved in ft_plot_cx,
// which is searched after ft_plot_cur when resolving vectors.  When the
// script ends, ft_plot_cx takes the prior value, which is zero at the top
// level.


// Call before executing a script, set the context plot to the curent plot
//
void
IFsimulator::PushPlot()
{
    ft_cxplots = new sPlotList(ft_plot_cx, ft_cxplots);
    ft_plot_cx = ft_plot_cur;
}


// Call after executing a script, restore the previous context plot
//
void
IFsimulator::PopPlot()
{
    ft_plot_cx = 0;
    if (ft_cxplots) {
        ft_plot_cx = ft_cxplots->plot;
        sPlotList *ptmp = ft_cxplots;
        ft_cxplots = ft_cxplots->next;
        delete ptmp;
    }
}


// Destroy the named plot.  If all_for_cir is true, also remove any
// other plots with the same circuit name (as saved in the title field).
// This is used by the Xic ipc interface.
//
void
IFsimulator::RemovePlot(const char *name, bool all_for_cir)
{
    if (!name)
        return;
    ToolBar()->UpdatePlots(1);
    if (lstring::cieq(name, "all")) {
        sPlot *npl;
        bool active = false;
        for (sPlot *pl = ft_plot_list; pl; pl = npl) {
            npl = pl->next_plot();
            if (pl->active())
                active = true;
            if (pl != sPlot::constants() && !pl->active())
                pl->destroy();
        }
        if (active)
            GRpkgIf()->ErrPrintf(ET_WARN, "Active plot(s) not deleted.\n");
        ft_plot_cur = sPlot::constants();
    }
    else  {
        if (all_for_cir) {
            char *cname = 0;
            for (sPlot *pl = ft_plot_list; pl; pl = pl->next_plot()) {
                if (lstring::eq(pl->type_name(), name)) {
                    if (pl == sPlot::constants()) {
                        GRpkgIf()->ErrPrintf(ET_WARN,
                            "Constants plot not deleted.\n");
                        return;
                    }
                    cname = lstring::copy(pl->title());
                    break;
                }
            }
            if (cname) {
                sPlot *npl;
                bool active = false;
                for (sPlot *pl = ft_plot_list; pl; pl = npl) {
                    npl = pl->next_plot();
                    if (pl->title() && lstring::eq(cname, pl->title())) {
                        if (pl->active())
                            active = true;
                        else if (pl != sPlot::constants())
                            pl->destroy();
                    }
                }
                if (active)
                    GRpkgIf()->ErrPrintf(ET_WARN,
                        "Active plot(s) not deleted.\n");
                delete [] cname;
            }
        }
        else {
            sPlot *pl;
            for (pl = ft_plot_list; pl; pl = pl->next_plot()) {
                if (lstring::eq(pl->type_name(), name))
                    break;
            }
            if (pl && !pl->active() && pl != sPlot::constants())
                pl->destroy();
            else if (pl && pl->active())
                GRpkgIf()->ErrPrintf(ET_WARN, "Active plot not deleted.\n");
            else if (pl == sPlot::constants())
                GRpkgIf()->ErrPrintf(ET_WARN, "Constants plot not deleted.\n");
        }
    }
    ToolBar()->UpdatePlots(1);
}


void
IFsimulator::RemovePlot(sPlot *plot)
{
    if (plot->active()) {
        GRpkgIf()->ErrPrintf(ET_MSG,
            "Can't delete active plot, reset analysis first.\n");
        return;
    }
    if (plot == sPlot::constants()) {
        GRpkgIf()->ErrPrintf(ET_MSG, "Can't delete constants plot.\n");
        return;
    }
    plot->destroy();
}


// This function will match "op" with "op1", but not "op1" with "op12".
//
bool
IFsimulator::PlotPrefix(const char *pre, const char *str)
{
    if (!*pre)
        return (true);
    while (*pre && *str) {
        if ((isupper(*pre) ? tolower(*pre) : *pre) !=
                (isupper(*str) ? tolower(*str) : *str))
            break;
        pre++;
        str++;
    }
    if (*pre || (*str && isdigit(pre[-1])))
        return (false);
    else
        return (true);
}


// Clear out the temporary vectors linked into the plot.  If purge_temp
// is true, delete the vectors with the VF_TEMPORARY flag set.  We can
// use this to more locally control vector purging:
//
//   sDataVec::set_temporary(true);
//    ...
//   sDataVec::set_temporary(false);
//   Sp.VecGc(true);
//
void
IFsimulator::VecGc(bool purge_temp)
{
    for (sPlot *pl = ft_plot_list; pl; pl = pl->next_plot()) {
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
                        wl->free();
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


// This is something we do in a few places...  Since vectors get copied a
// lot, we can't just compare pointers to tell if two vectors are 'really'
// the same.
//
bool
IFsimulator::VecEq(sDataVec *v1, sDataVec *v2)
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
IFsimulator::VecPrintList(wordlist *wl, char **retstr)
{
    char buf[BSIZE_SP];
    while (wl) {
        char *s = lstring::copy(wl->wl_word);
        CP.Unquote(s);
        sCKT *ckt = ft_curckt ? ft_curckt->runckt() : 0;
        sDataVec *d = VecGet(s, ckt);
        delete [] s;
        if (d == 0)
            Error(E_NOVEC, 0, wl->wl_word);
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
    if (!ft_plot_cur) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no current plot.\n");
        return;
    }

    wordlist *tl0 = ft_plot_cur->list_perm_vecs();
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
        tl0->sort();
    sprintf(buf, "Title: %s\n",  ft_plot_cur->title());
    sprintf(buf + strlen(buf), "Name: %s (%s)\nDate: %s\n\n", 
        ft_plot_cur->type_name(), ft_plot_cur->name(), ft_plot_cur->date());
    if (retstr)
        *retstr = lstring::build_str(*retstr, buf);
    else
        TTY.send(buf);

    // list scale first
    if (!ft_plot_cur->scale()) {
        for (wordlist *tl = tl0; tl; tl = tl->wl_next) {
            sDataVec *d = ft_plot_cur->get_perm_vec(tl->wl_word);
            d->print(retstr);
        }
    }
    else {
        for (wordlist *tl = tl0; tl; tl = tl->wl_next) {
            if (lstring::eq(tl->wl_word, ft_plot_cur->scale()->name())) {
                sDataVec *d = ft_plot_cur->get_perm_vec(tl->wl_word);
                d->print(retstr);
                break;
            }
        }
        for (wordlist *tl = tl0; tl; tl = tl->wl_next) {
            if (!lstring::eq(tl->wl_word, ft_plot_cur->scale()->name())) {
                sDataVec *d = ft_plot_cur->get_perm_vec(tl->wl_word);
                d->print(retstr);
            }
        }
    }
    tl0->free();
}
// End of IFsimulator functions


// Create a new plot structure. This fills in the strings and sets up
// the ccom struct if a name is passed.
//
sPlot::sPlot(const char *n)
{
    pl_title = 0;
    pl_date = 0;
    pl_name = 0;
    pl_typename = 0;
    pl_circuit = 0;
    pl_notes = 0;

    pl_next = 0;
    pl_hashtab = 0;
    pl_dvecs = 0;
    pl_scale = 0;
    pl_commands = 0;
    pl_env = 0;
    pl_ccom = 0;
    pl_dims = 0;
    pl_ftopts = 0;

    pl_start = 0.0;
    pl_stop = 0.0;
    pl_step = 0.0;

    pl_ndims = 0;
    pl_active = false;
    pl_written = false;

    if (n && *n) {
        const char *s;
        char buf[BSIZE_SP];
        if (!(s = Sp.PlotAbbrev(n)))
            s = "unknown";
        int plot_num = 1;
        for (sPlot *tp = Sp.PlotList(); tp; tp = tp->pl_next) {
            if (lstring::ciprefix(s, tp->pl_typename)) {
                int num = atoi(tp->pl_typename + strlen(s));
                if (num >= plot_num)
                    plot_num = num+1;
            }
        }
        sprintf(buf, "%s%d", s, plot_num);
        pl_typename = lstring::copy(buf);
        set_date(datestring());
        set_name(n);
        set_title("untitled");
        CP.AddKeyword(CT_PLOT, buf);
    }
}


sPlot::~sPlot()
{
    if (this == pl_constants)
        return;
    // clear temporary dvecs
    sDataVec *v, *nv;
    for (v = pl_dvecs; v; v = nv) {
        nv = v->next();
        delete v;
    }

    // clear permanent dvecs
    remove_vec("all");
    delete pl_hashtab;
    pl_env->free();

    // unlink plot from main list
    sPlot *op = 0;
    for (sPlot *pl = Sp.PlotList(); pl; pl = pl->pl_next) {
        if (pl == this) {
            if (!op) {
                Sp.SetPlotList(pl->pl_next);
                if (pl == Sp.CurPlot())
                    Sp.SetCurPlot(Sp.PlotList());
            }
            else {
                op->pl_next = pl->pl_next;
                if (pl == Sp.CurPlot())
                    Sp.SetCurPlot(op);
            }
            break;
        }
        op = pl;
    }

    // remove from context
    if (Sp.CxPlot() == this)
        Sp.SetCxPlot(0);
    for (sPlotList *p = Sp.CxPlotList(); p; p = p->next)
        if (p->plot == this)
            p->plot = 0;

    delete pl_ccom;
    delete [] pl_title;
    delete [] pl_date;
    delete [] pl_name;
    delete [] pl_typename;
    delete pl_dims;
    delete pl_circuit;
    delete pl_ftopts;
    pl_commands->free();
    pl_notes->free();
}


// Add v to the plot as a permanent vector.
//
void
sPlot::new_perm_vec(sDataVec *v)
{
    if (!v)
        return;
    if (pl_scale == 0)
        pl_scale = v;
    if (pl_hashtab == 0)
        pl_hashtab = new sHtab(sHtab::get_ciflag(CSE_VEC));
     
    // delete any vector with the same name
    sDataVec *vx = (sDataVec*)pl_hashtab->remove(v->name());
    if (vx) {
        if (pl_scale == vx)
            pl_scale = v;

        // This vector might be used as a scale by another vector,
        // have to find and fix these references.
        for (sPlot *px = Sp.PlotList(); px; px = px->next_plot()) {
            sHgen gen(px->pl_hashtab);
            sHent *h;
            while ((h = gen.next()) != 0) {
                sDataVec *t = (sDataVec*)h->data();
                if (t->scale() == vx)
                    t->set_scale(v);
            }
        }
        delete vx;
    }
    pl_hashtab->add(v->name(), v);
}


// Return a permanent vector from the hash table.
//
sDataVec *
sPlot::get_perm_vec(const char *word) const
{
    if (!pl_hashtab)
        return (0);
    if (!word || !*word)
        return (0);
    return ((sDataVec*)pl_hashtab->get(word));
}


// Return a list of permanent vector names from the hash table.
//
wordlist *
sPlot::list_perm_vecs() const
{
    if (!pl_hashtab)
        return (0);
    return (pl_hashtab->wl());
}


// Return the number of permanent vectors in the plot.
//
int
sPlot::num_perm_vecs() const
{
    if (!pl_hashtab)
        return (0);
    return (pl_hashtab->allocated());
}


// Get a vector by name.  This handles "all" and tries resolving as v(xxx)
// and i(xxx).
//
sDataVec *
sPlot::find_vec(const char *word)
{
    const sPlot *tplot = this;
    if (!tplot || !word || !*word)
        return (0);

    if (lstring::cieq(word, "all")) {
        // Create a dummy vector with link2 pointing at dvlist.
        sHgen gen(pl_hashtab);
        sHent *h;
        sDvList *dl0 = 0, *dl = 0;;
        while ((h = gen.next()) != 0) {
            sDataVec *d = (sDataVec*)h->data();
            // try to avoid returning the scale
            if (d == pl_scale && d->length() > 1 &&
                    pl_hashtab->allocated() > 1)
                continue;
            if (!dl0)
                dl = dl0 = new sDvList;
            else {
                dl->dl_next = new sDvList;
                dl = dl->dl_next;
            }
            dl->dl_dvec = d;
        }
        if (dl0) {
            sDataVec *d = new sDataVec(lstring::copy("list"), 0, 0);
            d->set_link(dl0);
            d->newtemp(this);
            if (!Sp.GetVar(kw_nosort, VTYP_BOOL, 0))
                d->sort();
            return (d);
        }
        return (0);
    }

    sDataVec *d = get_perm_vec(word);
    if (!d) {
        char buf[BSIZE_SP];
        if (!strchr(word, '(')) {
            buf[0] = 'v';
            buf[1] = '(';
            char *s = buf+2;
            while (*word)
                *s++ = *word++;
            *s++ = ')';
            *s = '\0';
            d = get_perm_vec(buf);
        }
        else if ((*word == 'i' || *word == 'I') && *(word + 1) == '(' &&
                *(word + strlen(word) - 1) == ')') {
            strcpy(buf, word + 2);
            strcpy(buf + strlen(buf) - 1, "#branch");
            d = get_perm_vec(buf);
        }
    }
    return (d);
}


// Remove and return the permanent vector.
//
sDataVec *
sPlot::remove_perm_vec(const char *word)
{
    if (!pl_hashtab)
        return (0);
    if (!word || !*word)
        return (0);
    return ((sDataVec*)pl_hashtab->remove(word));
}


// Remove a permanent vector from the plot, if it is there.  If the
// vname is "all", clear all vectors.  Removed vectors are destroyed.
//
void
sPlot::remove_vec(const char *vname)
{
    const char *msg = "can't remove %s from %s plot.\n";
    if (lstring::cieq(vname, "all")) {
        if (this == pl_constants) {
            GRpkgIf()->ErrPrintf(ET_ERROR, msg, vname, "constants");
            return;
        }
        if (pl_active) {
            GRpkgIf()->ErrPrintf(ET_ERROR, msg, vname, "active");
            return;
        }

        // The vectors might be used as a scale by another vector,
        // have to find and zero these references.
        for (sPlot *pl = Sp.PlotList(); pl; pl = pl->pl_next) {
            if (pl == this)
                continue;
            sHgen gen(pl->pl_hashtab);
            sHent *h;
            while ((h = gen.next()) != 0) {
                sDataVec *v = (sDataVec*)h->data();
                if (v->scale() && v->scale()->plot() == this)
                    v->set_scale(0);
            }
        }

        wordlist *wl0 = pl_hashtab->wl();
        for (wordlist *wl = wl0; wl; wl = wl->wl_next) {
            sDataVec *v = (sDataVec*)pl_hashtab->remove(wl->wl_word);
            delete v;
            if (this == Sp.CurPlot())
                CP.RemKeyword(CT_VECTOR, wl->wl_word);
        }
        pl_scale = 0;
        wl0->free();
        if (this == Sp.CurPlot())
            ToolBar()->UpdateVectors(0);
        return;
    }
    sDataVec *v = get_perm_vec(vname);
    if (!v)
        return;
    if (this == pl_constants && (v->flags() & VF_READONLY)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, msg, vname, "constants");
        return;
    }
    if (pl_active) {
        GRpkgIf()->ErrPrintf(ET_ERROR, msg, vname, "active");
        return;
    }
    pl_hashtab->remove(vname);
    if (pl_scale == v) {
        pl_scale = 0;
        wordlist *wl0 = pl_hashtab->wl();
        if (wl0) {
            pl_scale = get_perm_vec(wl0->wl_word);
            wl0->free();
        }
    }

    // This vector might be used as a scale by another vector,
    // have to find and zero these references.
    for (sPlot *pl = Sp.PlotList(); pl; pl = pl->pl_next) {
        sHgen gen(pl->pl_hashtab);
        sHent *h;
        while ((h = gen.next()) != 0) {
            sDataVec *t = (sDataVec*)h->data();
            if (t->scale() == v)
                t->set_scale(0);
        }
    }
    delete v;

    if (this == Sp.CurPlot()) {
        CP.RemKeyword(CT_VECTOR, vname);
        ToolBar()->UpdateVectors(0);
    }
}


int
sPlot::num_perm_vecs()
{
    if (!pl_hashtab)
        return (0);
    return (pl_hashtab->allocated());
}


// Change the case sensitivity flag fom perm vecs.
//
void
sPlot::set_case(bool ci)
{
    if (pl_hashtab)
        pl_hashtab->chg_ciflag(ci);
}


// Simply add a plot to the plot list.
//
void
sPlot::new_plot()
{
    pl_next = Sp.PlotList();
    Sp.SetPlotList(this);
}


// Add the plot to the list of plots.  This takes care of various
// things, and prints an announcement.
//
void
sPlot::add_plot()
{
    TTY.printf("Title:  %s\nName: %s\nDate: %s\n", pl_title, pl_name, pl_date);
    int plot_num = 1;
    const char *s;
    char buf[BSIZE_SP];
    if (!(s = Sp.PlotAbbrev(pl_name)))
        s = "unknown";
    for (sPlot *tp = Sp.PlotList(); tp; tp = tp->pl_next) {
        if (lstring::ciprefix(s, tp->pl_typename)) {
            int n = atoi(tp->pl_typename + strlen(s));
            if (n >= plot_num)
                plot_num = n+1;
        }
    }
    sprintf(buf, "%s%d", s, plot_num);
    pl_typename = lstring::copy(buf);
    new_plot();
    Sp.SetCurPlot(pl_typename);

    CP.AddKeyword(CT_PLOT, buf);
    wordlist *wl0 = pl_hashtab->wl();
    wordlist *wl = wl0;
    while (wl) {
        CP.AddKeyword(CT_VECTOR, wl->wl_word);
        wl = wl->wl_next;
    }
    wl0->free();
}


// If pl contains the same vectors and scale, add the data and bump up
// the dimensionality.
//
bool
sPlot::add_segment(const sPlot *pl)
{
    if (!compare(pl))
        return (false);
    wordlist *wl0 = pl_hashtab->wl();
    int slen = -1;
    sDataVec *xs = pl_scale;
    if (xs)
        slen = xs->dims(xs->numdims()-1);
    for (wordlist *wl = wl0; wl; wl = wl->wl_next) {
        sDataVec *d0 = get_perm_vec(wl->wl_word);
        sDataVec *d1 = pl->get_perm_vec(wl->wl_word);
        if (d0->isreal()) {
            double *dn = new double[d0->length() + d1->length()];
            memcpy(dn, d0->realvec(), d0->length()*sizeof(double));
            memcpy(dn + d0->length(), d1->realvec(),
                d1->length()*sizeof(double));
            d0->set_realvec(dn, true);
        }
        else {
            complex *cn = new complex[d0->length() + d1->length()];
            memcpy(cn, d0->compvec(), d0->length()*sizeof(complex));
            memcpy(cn + d0->length(), d1->compvec(),
                d1->length()*sizeof(complex));
            d0->set_compvec(cn, true);
        }
        d0->set_length(d0->length() + d1->length());

        // If the vector segment length is less than the scale segment
        // length, it is probably from a .measure or is some other scalar
        // type of variable.  In this case, just extend the length and
        // keep the dimensionality of one.
        if (d0->dims(d0->numdims()-1) < slen)
            continue;

        if (d1->numdims() == d0->numdims()) {
            d0->set_numdims(d0->numdims() + 1);;
            for (int k = 0; k < d1->numdims(); k++)
                d0->set_dims(k+1, d0->dims(k));
            d0->set_dims(0, 2);
        }
        else if (d1->numdims() == d0->numdims() - 1) {
            d0->set_dims(0, d0->dims(0) + 1);;
        }
        else if (d1->numdims() == d0->numdims() + 1) {
            for (int k = 0; k < d1->numdims(); k++)
                d0->set_dims(k, d1->dims(k));
            d0->set_dims(0, d0->dims(0) + 1);
        }
    }
    return (true);
}


// Execute the commands for a plot.  This is done whenever a plot
// becomes the current plot.
//
void
sPlot::run_commands()
{
    Sp.ExecCmds(pl_commands);
}


// Return true if pl contains the same vector names with the same data
// types and similar dimensionality.
//
bool
sPlot::compare(const sPlot *pl)
{
    wordlist *wl0 = pl_hashtab->wl();
    for (wordlist *wl = wl0; wl; wl = wl->wl_next) {
        sDataVec *d0 = get_perm_vec(wl->wl_word);
        sDataVec *d1 = pl->get_perm_vec(wl->wl_word);
        if (!d0 || !d1 || d0->flags() != d1->flags()) {
            wl0->free();
            return (false);
        }
        if (d0->numdims() < 1) {
            d0->set_numdims(1);
            d0->set_dims(0, d0->length());
        }
        if (d1->numdims() < 1) {
            d1->set_numdims(1);
            d1->set_dims(0, d1->length());
        }
        if (abs(d0->numdims() - d1->numdims()) > 1) {
            wl0->free();
            return (false);
        }
        int j = d0->numdims() - 1;
        int k = d1->numdims() - 1;
        for (;;) {
            if (d0->dims(j) != d1->dims(k)) {
                wl0->free();
                return (false);
            }
            j--;
            k--;
            if (j < 0 || k < 0)
                break;
        }
    }
    wl0->free();
    return (true);
}


// This is the recommended entry to destroy a plot.
//
void
sPlot::destroy()
{
    const sPlot *tplot = this;
    if (!tplot)
        return;

    if (this == pl_constants) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't destroy the constants plot.\n");
        return;
    }
    sPlot *pl = Sp.PlotList();
    for ( ; pl; pl = pl->pl_next)
        if (pl == this)
            break;
    if (!pl)
        GRpkgIf()->ErrPrintf(ET_INTERR, "plot_delete: not in list.\n");

    delete this;
    ToolBar()->UpdatePlots(0);
}


// Clear all VF_SELECTED flags in permanent vectors.
// This flag is not really used anywhere, but is set/cleared from the
// Vectors tool.
//
void
sPlot::clear_selected()
{
    if (pl_hashtab) {
        sHgen gen(pl_hashtab);
        sHent *h;
        while ((h = gen.next()) != 0) {
            sDataVec *dv = (sDataVec*)h->data();
            dv->set_flags(dv->flags() & ~VF_SELECTED);
        }
    }
}


// Append a note string to the plot.
//
void
sPlot::add_note(const char *n)
{
    if (!n || !*n)
        return;
    if (pl_notes) {
        wordlist *w = pl_notes;
        while (w->wl_next)
            w = w->wl_next;
        w->wl_next = new wordlist(n, w);
        return;
    }
    pl_notes = new wordlist(n, 0);
}


bool
sPlot::set_dims(int ndims, const int *dims)
{

    if (!pl_scale) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "plot has no scale.");
        return (false);
    }
    int len = pl_scale->length();

    int d = 1;
    for (int i = 1; i < ndims; i++)
        d *= dims[i-1];
    if (len % d) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "dimensions are not compatible with point count.");
        return (false);
    }

    sHgen gen(pl_hashtab);
    sHent *ent;
    while ((ent = gen.next()) != 0) {
        sDataVec *dv = (sDataVec*)ent->data();
        if (dv->length() == len) {
            dv->set_numdims(ndims);
            for (int i = 0; i < MAXDIMS; i++)
                dv->set_dims(i, dims[i]);
        }
    }
    return (true);
}
// End of sPlot functions

