
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
#include "output.h"
#include "cshell.h"
#include "commands.h"
#include "toolbar.h"
#include "datavec.h"
#include "rawfile.h"
#include "csdffile.h"
#include "prntfile.h"
#include "spnumber/hash.h"
#include "kwords_fte.h"
#include "optdefs.h"

//
// sPlot structure functions.
//


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
        for (sPlot *p = OP.plotList(); p; p = p->next_plot()) {
            if (OP.curPlot() == p)
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
    OP.setCurPlot(buf);
    if (wl)
        TTY.printf("%s %s (%s)\n", OP.curPlot()->type_name(),
            OP.curPlot()->title(), OP.curPlot()->name());
    if (OP.curPlot()->circuit()) {
        Sp.SetCircuit(OP.curPlot()->circuit());
        Sp.OptUpdate();
        TTY.printf("current circuit: %s, %s\n", Sp.CurCircuit()->name(),
            Sp.CurCircuit()->descr());
    }
}


// setdim numdims dim0 dim1 ...
// numdims-1 dims given
//
void
CommandTab::com_setdim(wordlist *wl)
{

    sPlot *p = OP.curPlot();
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


// Command to combine the last two "similar" plots into a single
// multi-dimensional plot.
//
void
CommandTab::com_combine(wordlist*)
{
    if (!OP.plotList()) {
        GRpkgIf()->ErrPrintf(ET_INTERR, "no plots in list!\n");
        return;
    }
    if (OP.plotList() == OP.constants()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't combine constants plot.\n");
        return;
    }
    if (!OP.plotList()->next_plot()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no plot to combine.\n");
        return;
    }
    if (OP.plotList()->next_plot() == OP.constants()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't combine constants plot.\n");
        return;
    }

    char *errstr;
    if (OP.plotList()->next_plot()->add_segment(OP.plotList(), &errstr))
        OP.plotList()->destroy();
    else {
        if (*errstr) {
            GRpkgIf()->ErrPrintf(ET_ERROR, 
                "%s,\nincompatible plots, can't combine.\n", errstr);
            delete [] errstr;
        }
        else {
            GRpkgIf()->ErrPrintf(ET_ERROR, 
                "incompatible plots, can't combine.\n");
        }
    }
}


void
CommandTab::com_destroy(wordlist *wl)
{
    if (!wl)
        OP.removePlot(OP.curPlot());
    else if (lstring::cieq(wl->wl_word, "all"))
        OP.removePlot("all");
    else {
        ToolBar()->UpdatePlots(1);
        while (wl) {
            sPlot *pl;
            for (pl = OP.plotList(); pl; pl = pl->next_plot())
                if (lstring::eq(pl->type_name(), wl->wl_word))
                    break;
            if (pl)
                OP.removePlot(pl);
            else
                GRpkgIf()->ErrPrintf(ET_ERROR, "no such plot %s.\n",
                    wl->wl_word);
            wl = wl->wl_next;
        }
        ToolBar()->UpdatePlots(1);
    }
}
// End of CommandTab functions.


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
IFoutput::findPlot(const char *name)
{
    if (name[0] == '-' && isdigit(name[1])) {
        int n = atoi(name+1);
        if (n < 1)
            return (o_plot_cur);
        for (sPlot *pl = o_plot_cur; pl; pl = pl->next_plot()) {
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
            return (o_plot_cur);
        for (sPlot *pl = o_plot_cur; pl; pl = pl->next_plot()) {
            if (!n--)
                return (pl);
        }
    }
    else if (name[0] == '+' && isdigit(name[1])) {
        int n = atoi(name+1);
        if (n < 1)
            return (o_plot_cur);
        int c = 0;
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot()) {
            if (pl == o_plot_cur)
                break;
            c++;
        }
        if (c < n)
            return (0);
        c -= n;
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot()) {
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
            return (o_plot_cur);
        int c = 0;
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot()) {
            if (pl == o_plot_cur)
                break;
            c++;
        }
        if (c < n)
            return (0);
        c -= n;
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot()) {
            if (!c--)
                return (pl);
        }
    }
    else if (isdigit(*name)) {
        int c = 0;
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot())
            c++;
        c--;
        int n = atoi(name);
        if (c < n)
            return (0);
        c -= n;
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot()) {
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
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot())
            c++;
        c--;
        if (c < n)
            return (0);
        c -= n;
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot()) {
            if (!c--)
                return (pl);
        }
    }
    else if (!strcmp(name, "curplot")) {
        return (o_plot_cur);
    }
    else {
        for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot()) {
            if (plotPrefix(name, pl->type_name()))
                return (pl);
        }
    }
    return (0);
}


// Make a plot the current one.  This gets called by cp_usrset() when one
// does a 'set curplot = name'.
//
void
IFoutput::setCurPlot(const char *name)
{
    if (lstring::cieq(name, "new")) {
        sPlot *pl = new sPlot("unknown");
        pl->new_plot();
        setCurPlot(pl);
    }
    else {
        sPlot *pl = findPlot(name);
        if (!pl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "can't find plot named %s.\n",
                name);
            return;
        }
        setCurPlot(pl);
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
// Now, before executing a script, the current plot is saved in OP.cxPlot(),
// which is searched after OP.curPlot() when resolving vectors.  When the
// script ends, OP.cxPlot() takes the prior value, which is zero at the top
// level.


// Call before executing a script, set the context plot to the curent plot
//
void
IFoutput::pushPlot()
{
    o_cxplots = new sPlotList(o_plot_cx, o_cxplots);
    o_plot_cx = o_plot_cur;
}


// Call after executing a script, restore the previous context plot
//
void
IFoutput::popPlot()
{
    o_plot_cx = 0;
    if (o_cxplots) {
        o_plot_cx = o_cxplots->plot;
        sPlotList *ptmp = o_cxplots;
        o_cxplots = o_cxplots->next;
        delete ptmp;
    }
}


// Destroy the named plot.  If all_for_cir is true, also remove any
// other plots with the same circuit name (as saved in the title field).
// This is used by the Xic ipc interface.
//
void
IFoutput::removePlot(const char *name, bool all_for_cir)
{
    if (!name)
        return;
    ToolBar()->UpdatePlots(1);
    if (lstring::cieq(name, "all")) {
        sPlot *npl;
        bool active = false;
        for (sPlot *pl = o_plot_list; pl; pl = npl) {
            npl = pl->next_plot();
            if (pl->active())
                active = true;
            if (pl != o_constants && !pl->active())
                pl->destroy();
        }
        if (active)
            GRpkgIf()->ErrPrintf(ET_WARN, "Active plot(s) not deleted.\n");
        setCurPlot(o_constants);
    }
    else  {
        if (all_for_cir) {
            char *cname = 0;
            for (sPlot *pl = o_plot_list; pl; pl = pl->next_plot()) {
                if (lstring::eq(pl->type_name(), name)) {
                    if (pl == o_constants) {
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
                for (sPlot *pl = o_plot_list; pl; pl = npl) {
                    npl = pl->next_plot();
                    if (pl->title() && lstring::eq(cname, pl->title())) {
                        if (pl->active())
                            active = true;
                        else if (pl != o_constants)
                            pl->destroy();
                    }
                }
                if (active) {
                    GRpkgIf()->ErrPrintf(ET_WARN,
                        "Active plot(s) not deleted.\n");
                }
                delete [] cname;
            }
        }
        else {
            sPlot *pl;
            for (pl = o_plot_list; pl; pl = pl->next_plot()) {
                if (lstring::eq(pl->type_name(), name))
                    break;
            }
            if (pl && !pl->active() && pl != o_constants)
                pl->destroy();
            else if (pl && pl->active())
                GRpkgIf()->ErrPrintf(ET_WARN, "Active plot not deleted.\n");
            else if (pl == o_constants)
                GRpkgIf()->ErrPrintf(ET_WARN, "Constants plot not deleted.\n");
        }
    }
    ToolBar()->UpdatePlots(1);
}


void
IFoutput::removePlot(sPlot *plot)
{
    if (plot->active()) {
        GRpkgIf()->ErrPrintf(ET_MSG,
            "Can't delete active plot, reset analysis first.\n");
        return;
    }
    if (plot == o_constants) {
        GRpkgIf()->ErrPrintf(ET_MSG, "Can't delete constants plot.\n");
        return;
    }
    plot->destroy();
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
IFoutput::loadFile(const char **fnameptr, bool written)
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


// Static function.
// This function will match "op" with "op1", but not "op1" with "op12".
//
bool
IFoutput::plotPrefix(const char *pre, const char *str)
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
// End of IFoutput functions.


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
    pl_fftsc_ix = 0;
    pl_active = false;
    pl_written = false;

    if (n && *n) {
        const char *s;
        char buf[BSIZE_SP];
        if (!(s = Sp.PlotAbbrev(n)))
            s = "unknown";
        int plot_num = 1;
        for (sPlot *tp = OP.plotList(); tp; tp = tp->pl_next) {
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
    if (this == OP.constants())
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
    variable::destroy(pl_env);

    // unlink plot from main list
    sPlot *op = 0;
    for (sPlot *pl = OP.plotList(); pl; pl = pl->pl_next) {
        if (pl == this) {
            if (!op) {
                OP.setPlotList(pl->pl_next);
                if (pl == OP.curPlot())
                    OP.setCurPlot(OP.plotList());
            }
            else {
                op->pl_next = pl->pl_next;
                if (pl == OP.curPlot())
                    OP.setCurPlot(op);
            }
            break;
        }
        op = pl;
    }

    // remove from context
    if (OP.cxPlot() == this)
        OP.setCxPlot(0);
    for (sPlotList *p = OP.cxPlotList(); p; p = p->next) {
        if (p->plot == this)
            p->plot = 0;
    }

    delete pl_ccom;
    delete [] pl_title;
    delete [] pl_date;
    delete [] pl_name;
    delete [] pl_typename;
    delete pl_dims;
    delete pl_circuit;
    delete pl_ftopts;
    wordlist::destroy(pl_commands);
    wordlist::destroy(pl_notes);
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
        for (sPlot *px = OP.plotList(); px; px = px->next_plot()) {
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
    return ((sDataVec*)sHtab::get(pl_hashtab, word));
}


// Return a list of permanent vector names from the hash table.
//
wordlist *
sPlot::list_perm_vecs() const
{
    if (!pl_hashtab)
        return (0);
    return (sHtab::wl(pl_hashtab));
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
        if (this == OP.constants()) {
            GRpkgIf()->ErrPrintf(ET_ERROR, msg, vname, "constants");
            return;
        }
        if (pl_active) {
            GRpkgIf()->ErrPrintf(ET_ERROR, msg, vname, "active");
            return;
        }

        // The vectors might be used as a scale by another vector,
        // have to find and zero these references.
        for (sPlot *pl = OP.plotList(); pl; pl = pl->pl_next) {
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

        wordlist *wl0 = sHtab::wl(pl_hashtab);
        for (wordlist *wl = wl0; wl; wl = wl->wl_next) {
            sDataVec *v = (sDataVec*)pl_hashtab->remove(wl->wl_word);
            delete v;
            if (this == OP.curPlot())
                CP.RemKeyword(CT_VECTOR, wl->wl_word);
        }
        pl_scale = 0;
        wordlist::destroy(wl0);
        if (this == OP.curPlot())
            ToolBar()->UpdateVectors(0);
        return;
    }
    sDataVec *v = get_perm_vec(vname);
    if (!v)
        return;
    if (this == OP.constants() && (v->flags() & VF_READONLY)) {
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
        wordlist *wl0 = sHtab::wl(pl_hashtab);
        if (wl0) {
            pl_scale = get_perm_vec(wl0->wl_word);
            wordlist::destroy(wl0);
        }
    }

    // This vector might be used as a scale by another vector,
    // have to find and zero these references.
    for (sPlot *pl = OP.plotList(); pl; pl = pl->pl_next) {
        sHgen gen(pl->pl_hashtab);
        sHent *h;
        while ((h = gen.next()) != 0) {
            sDataVec *t = (sDataVec*)h->data();
            if (t->scale() == v)
                t->set_scale(0);
        }
    }
    delete v;

    if (this == OP.curPlot()) {
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
    pl_next = OP.plotList();
    OP.setPlotList(this);
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
    for (sPlot *tp = OP.plotList(); tp; tp = tp->pl_next) {
        if (lstring::ciprefix(s, tp->pl_typename)) {
            int n = atoi(tp->pl_typename + strlen(s));
            if (n >= plot_num)
                plot_num = n+1;
        }
    }
    sprintf(buf, "%s%d", s, plot_num);
    pl_typename = lstring::copy(buf);
    new_plot();
    OP.setCurPlot(pl_typename);

    CP.AddKeyword(CT_PLOT, buf);
    wordlist *wl0 = sHtab::wl(pl_hashtab);
    wordlist *wl = wl0;
    while (wl) {
        CP.AddKeyword(CT_VECTOR, wl->wl_word);
        wl = wl->wl_next;
    }
    wordlist::destroy(wl0);
}


// If pl contains the same vectors and scale, add the data and bump up
// the dimensionality.
//
bool
sPlot::add_segment(const sPlot *pl, char **errstr)
{
    if (!compare(pl, errstr))
        return (false);

    wordlist *wl0 = sHtab::wl(pl_hashtab);
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
sPlot::compare(const sPlot *pl, char **errstr)
{
    if (errstr)
        *errstr = 0;
    wordlist *wl0 = sHtab::wl(pl_hashtab);
    for (wordlist *wl = wl0; wl; wl = wl->wl_next) {
        sDataVec *d0 = get_perm_vec(wl->wl_word);
        sDataVec *d1 = pl->get_perm_vec(wl->wl_word);
        if (!d0 || !d1 || d0->flags() != d1->flags()) {
            if (errstr) {
                sLstr lstr;
                lstr.add("vector ");
                lstr.add(wl->wl_word);
                lstr.add(" not found in both plots");
                *errstr = lstr.string_trim();
            }
            wordlist::destroy(wl0);
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
            wordlist::destroy(wl0);
            if (errstr)
                *errstr = lstring::copy("dimension counts are incompatible");
            return (false);
        }
        int j = d0->numdims() - 1;
        int k = d1->numdims() - 1;
        for (;;) {
            if (d0->dims(j) != d1->dims(k)) {
                wordlist::destroy(wl0);
                if (errstr)
                    *errstr = lstring::copy("vector lengths are incompatible");
                return (false);
            }
            j--;
            k--;
            if (j < 0 || k < 0)
                break;
        }
    }
    wordlist::destroy(wl0);
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

    if (this == OP.constants()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't destroy the constants plot.\n");
        return;
    }
    sPlot *pl = OP.plotList();
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

