
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
         1992 David A. Gates ( Xgraph() )
         1993 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "outplot.h"
#include "outdata.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "miscutil/filestat.h"


//
// Plotting functions.
//

namespace { bool asciilin(sDvList*, bool*); }


// asciiplot file name ... [vs xname]
//
void
CommandTab::com_asciiplot(wordlist *wl)
{
    GP.Plot(wl, 0, 0, "lpr", GR_PLOT);
}


// xgraph file name ... [vs xname]
//
void
CommandTab::com_xgraph(wordlist *wl)
{
    char *fname = 0;
    if (wl) {
        fname = wl->wl_word;
        wl = wl->wl_next;
    }
    if (!wl)
        return;
    bool tempf = false;
    if (lstring::cieq(fname, "temp") || lstring::cieq(fname, "tmp")) {
        fname = filestat::make_temp("xg");
        tempf = true;
    }
    GP.Plot(wl, 0, fname, "xgraph", GR_PLOT);
    if (tempf) {
        filestat::queue_deletion(fname);
        delete [] fname;
    }
}


// plot name ... [vs xname]
//
void
CommandTab::com_plot(wordlist *wl)
{
    if (!GRpkgIf()->CurDev() || GRpkgIf()->CurDev()->devtype == GRnodev ||
            GRpkgIf()->CurDev()->devtype == GRhardcopy) {
        // Do an ascii plot.
        GP.Plot(wl, 0, 0, "lpr", GR_PLOT);
        return;
    }
    GP.Plot(wl, 0, 0, 0, GR_PLOT);
}
// End of CommandTab functions.


// A command to allow popdown of plot windows from scripts.
//
void
CommandTab::com_plotwin(wordlist *wl)
{
    int d;
    if (!wl || *wl->wl_word == 'i' || *wl->wl_word == 'I')
        TTY.printf("%d\n", GP.RunningId() - 1);
    else if (*wl->wl_word == 'k' || *wl->wl_word == 'K') {
        wl = wl->wl_next;
        if (!wl) {
            // pop down last
            sGraph *graph = GP.FindGraph(GP.RunningId() - 1);
            if (graph)
                graph->gr_popdown();
        }
        else if (*wl->wl_word == 'a' || *wl->wl_word == 'A') {
            // pop down all plots
            for (int i = 1; i < GP.RunningId(); i++) {
                sGraph *graph = GP.FindGraph(i);
                if (graph)
                    graph->gr_popdown();
            }
        }
        else if (sscanf(wl->wl_word, "%d", &d) == 1) {
            if (d >= 0) {
                // pop down this id
                sGraph *graph = GP.FindGraph(d);
                if (graph)
                    graph->gr_popdown();
            }
            else {
                // pop down prev
                sGraph *graph = GP.FindGraph(GP.RunningId() - 1 + d);
                if (graph)
                    graph->gr_popdown();
            }
        }
        else {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "plotwin kill: unrecognized input.\n");
        }
    }
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "plotwin: unrecognized input.\n");
}


namespace {
    // Return true if the vec scale is the same as scale.
    //
    bool same_scale(sDataVec *vec, sDataVec *scale)
    {
        if (!scale || !vec)
            return (false);
        sDataVec *sc = vec->scale();
        if (!sc && vec->plot())
            sc = vec->plot()->scale();
        if (!sc && OP.curPlot())
            sc = OP.curPlot()->scale();
        if (!sc)
            return (false);
        if (sc == scale)
            return (true);
        if (sc->length() != scale->length())
            return (false);
        for (int i = 0; i < sc->length(); i++) {
            if (sc->realval(i) != scale->realval(i))
                return (false);
        }
        return (true);
    }
}


// The common routine for all plotting commands. This does hardcopy
// and graphics plotting.
//
bool
SPgraphics::Plot(wordlist *wl, sGraph *fromgraph, const char *hcopy,
    const char *devname, int)
{
    if (Sp.GetVar(kw_dontplot, VTYP_BOOL, 0)) {
        GRpkgIf()->ErrPrintf(ET_MSG,
            "No plotting when \"dontplot\" is set.\n");
        return (true);
    }

    static wordlist *wlast;
    sDvList *dl0;
    sDataVec *scale = 0;
    const char *plotattr = 0;
    if (!fromgraph) {
        if (!wl) {
            if (!wlast) {
                GRpkgIf()->ErrPrintf(ET_ERRORS, "no vectors given.\n");
                return (false);
            }
            wl = wlast;
        }
        else {
            wordlist::destroy(wlast);
            wlast = wordlist::copy(wl);
            wl = wlast;
            for (wordlist *ww = wl; ww; ww = ww->wl_next) {
                if (lstring::eq(ww->wl_word, ".")) {
                    wordlist *wx = Sp.ExtractPlotCmd(0, 0);
                    if (!wx) {
                        GRpkgIf()->ErrPrintf(ET_ERRORS,
                            "no vectors found for '.'.\n");
                        return (false);
                    }
                    if (ww == wl) {
                        wlast = wx;
                        wl = wx;
                    }
                    ww = ww->splice(wx);
                    continue;
                }
                if (ww->wl_word[0] == '.' && ww->wl_word[1] == '@' &&
                        isdigit(ww->wl_word[2])) {
                    int n = atoi(ww->wl_word + 2);
                    wordlist *wx = Sp.ExtractPlotCmd(n ? n-1 : n, 0);
                    if (!wx) {
                        GRpkgIf()->ErrPrintf(ET_ERRORS,
                            "no vectors found for '.@%d'.\n", n);
                        return (false);
                    }
                    if (ww == wl) {
                        wlast = wx;
                        wl = wx;
                    }
                    ww = ww->splice(wx);
                    continue;
                }
            }
        }

        // Flatten the command string, and remove any plot attribute
        // directives.  Plot attribute directives found in the plot
        // command line will override variables.

        bool plotall = false;
        sLstr ls_opt;
        sLstr ls_plot;
        char *string = wordlist::flatten(wl);
        const char *s = string;

        // This is a bit tricky.  We want to allow an optional '='
        // between keywords and values.  We also support N N and N,N
        // for the keywords that take two numbers.

        char *tok;
        const char *lastpos = s;
        while ((tok = lstring::getqtok(&s, "=")) != 0) {
            bool handled = false;
            if (lstring::cieq(tok, kw_xlimit) ||
                    lstring::cieq(tok, kw_ylimit) ||
                    lstring::cieq(tok, kw_xindices)) {

                char *tok1, *tok2;
                if (*s == '"') {
                    tok1 = lstring::gettok(&s, ",\"");
                    tok2 = lstring::gettok(&s, "\"");
                }
                else {
                    tok1 = lstring::gettok(&s, ",");
                    tok2 = lstring::gettok(&s);
                }
                if (tok1 && tok2) {
                    if (ls_opt.length())
                        ls_opt.add_c(' ');
                    ls_opt.add(tok);
                    ls_opt.add_c(' ');
                    ls_opt.add(tok1);
                    ls_opt.add_c(' ');
                    ls_opt.add(tok2);
                }
                delete [] tok1;
                delete [] tok2;
                handled = true;
            }
            else if (lstring::cieq(tok, kw_xcompress) ||
                    lstring::cieq(tok, kw_xdelta) ||
                    lstring::cieq(tok, kw_ydelta)) {

                char *tok1 = lstring::gettok(&s);
                if (tok1) {
                    if (ls_opt.length())
                        ls_opt.add_c(' ');
                    ls_opt.add(tok);
                    ls_opt.add_c(' ');
                    ls_opt.add(tok1);
                    ls_opt.add_c(' ');
                }
                delete [] tok1;
                handled = true;
            }
            else if (lstring::cieq(tok, kw_nointerp) ||
                    lstring::cieq(tok, kw_ysep) ||
                    lstring::cieq(tok, kw_noplotlogo) ||
                    lstring::cieq(tok, kw_nogrid) ||

                    lstring::cieq(tok, kw_lingrid) ||
                    lstring::cieq(tok, kw_xlog) ||
                    lstring::cieq(tok, kw_ylog) ||
                    lstring::cieq(tok, kw_loglog) ||
                    lstring::cieq(tok, kw_polar) ||
                    lstring::cieq(tok, kw_smith) ||
                    lstring::cieq(tok, kw_smithgrid) ||

                    lstring::cieq(tok, kw_linplot) ||
                    lstring::cieq(tok, kw_pointplot) ||
                    lstring::cieq(tok, kw_combplot) ||

                    lstring::cieq(tok, kw_multi) ||
                    lstring::cieq(tok, kw_single) ||
                    lstring::cieq(tok, kw_group)) {

                if (ls_opt.length())
                    ls_opt.add_c(' ');
                ls_opt.add(tok);
                handled = true;
            }
            else if (lstring::cieq(tok, kw_xlabel) ||
                    lstring::cieq(tok, kw_ylabel) ||
                    lstring::cieq(tok, kw_title)) {

                char *tok1 = lstring::getqtok(&s);
                if (tok1) {
                    if (ls_opt.length())
                        ls_opt.add_c(' ');
                    ls_opt.add(tok);
                    ls_opt.add_c(' ');
                    ls_opt.add_c('"');
                    ls_opt.add(tok1);
                    ls_opt.add_c('"');
                    ls_opt.add_c(' ');
                }
                delete [] tok1;
                handled = true;
            }
            delete [] tok;
            if (handled) {
                lastpos = s;
                continue;
            }

            const char *endpos = s;
            s = lastpos;
            tok = lstring::getqtok(&s, "=");
            if (lstring::cieq(tok, "all"))
                plotall = true;
            delete [] tok;

            // Not a keyword, back up and parse as an expression fragment.
            //
            // getqtok strips internal quoting, which is not ok.  Keep
            // the expression text verbatim.
            s = lastpos;
            while (isspace(*s))
                s++;
            if (ls_plot.length())
                ls_plot.add_c(' ');
            const char *e = endpos;
            while (e > s && isspace(*(e-1)))
                e--;
            while (s < e)
                ls_plot.add_c(*s++);
            s = endpos;
            lastpos = s;
        }
        delete [] string;

        // Now parse the vectors.  The argument list can contain the "vs"
        // clause, which indicates that the following vector is to be used
        // as the scale.  Since it's a bit of a hassle to parse the
        // vector boundaries here, we call Sp.GetPtree() without the check
        // flag.  The "vs" entry will have a nil dl_dvec.
        //
        // The plot arguments are converted back to a wordlist, since
        // GetPtree(wordlist*,...) handles quoted tokens properly, i.e.,
        // quoted tokens are evaluated as separate expressions.
        //
        wordlist *plotcmd = CP.Lexer(ls_plot.string());
        pnlist *pl0 = Sp.GetPtree(plotcmd, false);
        wordlist::destroy(plotcmd);
        if (pl0 == 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "Bad syntax: %s.\n",
                ls_plot.string());
            return (false);
        }

        dl0 = Sp.DvList(pl0);
        if (dl0 == 0)
            return (false);

        bool xyplot = false;  // true if "vs" found.
        sDvList *tl, *dl, *dn;
        for (tl = 0, dl = dl0; dl; tl = dl, dl = dn) {
            dn = dl->dl_next;
            sDataVec *d = dl->dl_dvec;
            if (!d) {
                // only from "vs"
                if (!tl || !dn) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "misplaced vs arg.\n");
                    sDvList::destroy(dl0);
                    return (false);
                }
                xyplot = true;
                scale = dn->dl_dvec;
                tl->dl_next = dn->dl_next;
                delete dl;
                delete dn;
                break;
            }
        }
        if (scale && !scale->length()) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s: no such vector.\n",
                scale->name());
            sDvList::destroy(dl0);
            return (false);
        }

        // Now check for 0-length vectors and extra vs's.  The 0-length
        // vectors should have already been stripped in DvList()
        //
        for (dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *d = dl->dl_dvec;
            if (!d) {
                // only from "vs", can only have one such, already found
                GRpkgIf()->ErrPrintf(ET_ERROR, "only one \"vs\" allowed.\n");
                sDvList::destroy(dl0);
                return (false);
            }
            if (!d->length()) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s: no such vector.\n",
                    d->name());
                sDvList::destroy(dl0);
                return (false);
            }
        }

        // Copy any permanent dvecs (there shouldn't be any)
        for (dl = dl0; dl; dl = dl->dl_next) {
            if (dl->dl_dvec->flags() & VF_PERMANENT) {
                dl->dl_dvec = dl->dl_dvec->copy();
                dl->dl_dvec->newtemp();
            }
        }

        if (!scale) {
            scale = dl0->dl_dvec->scale();
            if (!scale && dl0->dl_dvec->plot())
                scale = dl0->dl_dvec->plot()->scale();
            if (scale) {
                scale = scale->copy();
                scale->newtemp();
            }
            else
                scale = dl0->dl_dvec;
        }

        // The scale is not returned with "all", so add it to the list
        // if pole/zero data.
        if (plotall) {
            if (scale && (scale->flags() & (VF_POLE | VF_ZERO))) {
                for (dl = dl0; dl; dl = dl->dl_next)
                    if (dl->dl_dvec == scale)
                        break;
                if (!dl) {
                    dl = new sDvList;
                    dl->dl_dvec = scale;
                    dl->dl_next = dl0;
                    dl0 = dl;
                }
                scale = 0;
            }
        }

        // Interpolate to scale, if possible.
        if (scale && scale->length() > 2) {
            for (dl = dl0; dl; dl = dl->dl_next) {
                if (xyplot || dl->dl_dvec->length() <= 2 ||
                        same_scale(dl->dl_dvec, scale)) {
                    continue;
                }
                sDataVec *nv = dl->dl_dvec->v_interpolate(scale, true);
                if (!nv)
                    continue;

                if (dl->dl_dvec->isreal()) {
                    dl->dl_dvec->set_realvec(nv->realvec(), true);
                    nv->set_realvec(0);
                }
                else {
                    dl->dl_dvec->set_compvec(nv->compvec(), true);
                    nv->set_compvec(0);
                }
                dl->dl_dvec->set_length(nv->length());
                dl->dl_dvec->set_allocated(nv->allocated());
            }
        }

        plotattr = ls_opt.string_trim();
    }
    else
        dl0 = (sDvList*)fromgraph->plotdata();

    if (dl0 == 0) {
        delete [] plotattr;
        return (false);
    }

    if (devname &&
            (lstring::eq(devname, "xgraph") || lstring::eq(devname, "lpr"))) {
        // If there are higher dimensional vectors, transform them into a
        // family of vectors.
        //
        sDvList *tl = 0, *dn;
        for (sDvList *dl = dl0; dl; dl = dn) {
            dn = dl->dl_next;
            sDataVec *d = dl->dl_dvec;
            if (d->numdims() > 1) {
                sDataVec *lv = d->mkfamily();
                if (lv && lv->link()) {
                    if (!tl)
                        dl0 = lv->link();
                    else
                        tl->dl_next = lv->link();
                    sDvList *dx;
                    for (dx = lv->link(); dx->dl_next; dx = dx->dl_next) ;
                    dx->dl_next = dn;
                    tl = dx;
                    lv->set_link(0);  // or gets freed twice
                }
                else {
                    if (!tl)
                        dl0 = dn;
                    else
                        tl->dl_next = dn;
                }
                delete dl;
            }
            else
                tl = dl;
        }
    }

    sGrInit gr;
    sDvList *dl = dl0;
    bool ret = Setup(&gr, &dl0, plotattr, scale, hcopy);
    delete [] plotattr;
    if (!ret) {
        if (dl != dl0)
            sDvList::destroy(dl0);
        if (!fromgraph)
            sDvList::destroy(dl);
        return (false);
    }
    bool copied = false;
    if (dl != dl0) {
        // copy made
        copied = true;
        if (!fromgraph)
            sDvList::destroy(dl);
    }

    if (devname) {
        if (lstring::eq(devname, "xgraph")) {
            // Interface to XGraph-11 Plot Program
            Xgraph(dl0, &gr);
            if (!fromgraph)
                sDvList::destroy(dl0);
            return (true);
        }
        if (lstring::eq(devname, "lpr")) {
            // yecch, line printer plot
            if (!asciilin(dl0, &gr.nointerp)) {
                if (!fromgraph)
                    sDvList::destroy(dl0);
                return (false);
            }
            AsciiPlot(dl0, (char*)&gr);
            if (!fromgraph)
                sDvList::destroy(dl0);
            return (true);
        }
    }

    if (fromgraph)
        gr.command = wordlist::copy(fromgraph->command());
    else
        gr.command = wordlist::copy(wl);

    // the dvec list is copied to graph->plotdata
    sGraph *graph = Init(dl0, &gr);
    if (!fromgraph || copied)
        sDvList::destroy(dl0);

    if (graph == 0)
        return (false);

    // Propagate the saved text strings and attributes.
    if (fromgraph)
        graph->gr_update_keyed(fromgraph, true);

    if (GRpkgIf()->CurDev()->devtype == GRfullScreen ||
            GRpkgIf()->CurDev()->devtype == GRhardcopy) {
        graph->gr_end();
        graph->halt();
        DestroyGraph(graph->id());
    }
    return (true);
}


namespace {
    // Check if we should (can) linearize.
    //
    bool asciilin(sDvList *dl0, bool *nointerp)
    {
        sPlot *p = OP.curPlot();
        if (!p)
        if (!p || !p->scale() || !p->scale()->isreal())
            return (true);
        if (!lstring::ciprefix("tran", p->type_name()))
            return (true);
        double tstart, tstop, tstep;
        p->range(&tstart, &tstop, &tstep);
        if ((tstop - tstart)*tstep <= 0.0 || (tstop - tstart) < tstep)
            return (true);

        double *dd = p->scale()->realvec();
        double dt = dd[1] - dd[0];
        int i;
        for (i = 2; i < p->scale()->length(); i++) {
            if (fabs(dd[i] - dd[i-1] - dt) > .001*dt)
                break;
        }
        if (i == p->scale()->length())
            // already linear
            return (true);

        int newlen = (int)((tstop - tstart) / tstep + 1.5);
        double *newscale = new double[newlen];
        sDataVec *d = dl0->dl_dvec;

        sDataVec *newv_scale = new sDataVec(lstring::copy(d->scale()->name()),
            d->scale()->flags(), newlen, d->scale()->units(), newscale);
        newv_scale->set_gridtype(d->scale()->gridtype());
        newv_scale->newtemp();

        double ttime;
        for (i = 0, ttime = tstart; i < newlen; i++, ttime += tstep)
            newscale[i] = ttime;

        sDvList *dl;
        for (dl = dl0; dl; dl = dl->dl_next) {
            d = dl->dl_dvec;
            double *newdata = new double[newlen];

            sPoly po(1);
            if (!po.interp(d->realvec(), newdata, 
                    d->scale()->realvec(), d->length(), newscale, newlen)) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "can't interpolate %s.\n", d->name());
                return(false);
            }

            d->set_realvec(newdata, true);

            *nointerp = true;
        }
        dl0->dl_dvec->set_scale(newv_scale);
        return (true);
    }
}

